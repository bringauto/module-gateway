#include <bringauto/common_utils/EnumUtils.hpp>
#include <bringauto/external_client/connection/communication/QuicCommunication.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/settings/LoggerId.hpp>

#include <span>
#include <unordered_set>


namespace bringauto::external_client::connection::communication {
	/**
	 * @brief Initializes QUIC communication for the specified endpoint and vehicle.
	 *
	 * Invalid QUIC settings or transport initialization failures leave the channel
	 * uninitialized.
	 *
	 * @param settings External connection and QUIC transport settings.
	 * @param company Company associated with the connection.
	 * @param vehicleName Vehicle associated with the connection.
	 */
	QuicCommunication::QuicCommunication(const structures::ExternalConnectionSettings &settings,
	                                     const std::string &company,
	                                     const std::string &vehicleName) : ICommunicationChannel(settings) {
		bringauto::quic::QuicClientCallbacks callbacks;
		callbacks.onConnected = [this]() { onConnected(); };
		callbacks.onDisconnected = [this]() { onDisconnected(); };
		callbacks.onShutdownInitiatedByPeer = [this](QUIC_UINT62) { onShutdownInitiatedByPeer(); };
		callbacks.onBytesReceived = [this](std::vector<std::uint8_t> bytes) { onBytesReceived(std::move(bytes)); };

		bringauto::quic::QuicSettings quicSettings;
		try {
			quicSettings = buildQuicSettings(settings);
		} catch (const nlohmann::json::exception &e) {
			settings::Logger::logCritical("[quic] Invalid QUIC settings in config: {}", e.what());
			return;
		}

		quicClient_ = std::make_unique<bringauto::quic::QuicClient>(buildEndpointConfig(settings),
		                                                            std::move(quicSettings), std::move(callbacks));
		initialized_ = quicClient_->initialize();
		if (!initialized_) {
			settings::Logger::logCritical("[quic] Transport initialize() failed");
		}

		settings::Logger::logInfo("[quic] Initialize QUIC communication to {}:{} for {}/{}", settings.serverIp,
		                          settings.port, company, vehicleName);
	}

	/**
	 * @brief Stops communication and releases the QUIC client resources.
	 */
	QuicCommunication::~QuicCommunication() {
		stop();
	}

	/**
	 * @brief Builds the QUIC endpoint configuration from external connection settings.
	 *
	 * @param settings Connection settings containing the server address, port, and TLS parameters.
	 * @return QUIC endpoint configuration populated with the connection and certificate settings.
	 */
	bringauto::quic::QuicEndpointConfig QuicCommunication::buildEndpointConfig(
		const structures::ExternalConnectionSettings &settings
	) {
		bringauto::quic::QuicEndpointConfig config;
		config.host = settings.serverIp;
		config.port = settings.port;
		config.alpn = getProtocolSettingsString(settings, settings::Constants::ALPN);
		config.certPath = getProtocolSettingsString(settings, settings::Constants::CLIENT_CERT);
		config.keyPath = getProtocolSettingsString(settings, settings::Constants::CLIENT_KEY);
		config.caCertsPath = getProtocolSettingsString(settings, settings::Constants::CA_FILE);
		return config;
	}

	/**
	 * @brief Builds QUIC transport settings from external connection settings.
	 *
	 * @param settings Connection settings containing QUIC protocol options.
	 * @return Parsed QUIC transport settings.
	 */
	bringauto::quic::QuicSettings QuicCommunication::buildQuicSettings(
		const structures::ExternalConnectionSettings &settings
	) {
		static const std::unordered_set<std::string_view> endpointConfigKeys {
			settings::Constants::CA_FILE, settings::Constants::CLIENT_CERT, settings::Constants::CLIENT_KEY,
			settings::Constants::ALPN
		};

		nlohmann::json json = nlohmann::json::object();
		for (const auto &[key, raw]: settings.protocolSettings) {
			if (endpointConfigKeys.contains(key)) {
				continue;
			}
			// NOTE: SettingsParser stores string values unquoted, so a string-typed setting whose
			// value happens to look numeric/boolean/null (e.g. "1000") is indistinguishable here from
			// a genuinely numeric one and gets parsed as JSON rather than kept as a string. Every QUIC
			// setting is currently numeric, so this is harmless today; it would need a typed lookup
			// (not raw-string re-parsing) the moment a string-typed QUIC setting is added.
			json[key] = nlohmann::json::accept(raw) ? nlohmann::json::parse(raw) : nlohmann::json(raw);
		}

		std::vector<std::string> unrecognizedKeys;
		auto quicSettings = bringauto::quic::QuicSettings::fromJson(json, &unrecognizedKeys);
		for (const auto &key: unrecognizedKeys) {
			settings::Logger::logWarning("[quic] Unrecognized QUIC setting '{}', ignored", key);
		}
		return quicSettings;
	}

	/**
	 * @brief Establishes a QUIC connection and waits for the handshake to complete.
	 *
	 * Clears messages from any previous session before starting the connection attempt.
	 * If the connection cannot be established within the configured timeout, requests
	 * disconnection and restores the disconnected state.
	 */
	void QuicCommunication::initializeConnection() {
		cancelReceive_.store(false);

		settings::Logger::logDebug("[quic] Connecting to server when {}",
		                           common_utils::EnumUtils::connectionStateToString(connectionState_));

		ConnectionState expected = ConnectionState::NOT_CONNECTED;
		if (!connectionState_.compare_exchange_strong(expected, ConnectionState::CONNECTING)) {
			settings::Logger::logError("Connection already in progress or established");
			return;
		}

		// Now that we own a fresh connection attempt, drop any messages left over from a previous
		// session: on a reconnect a stale frame would otherwise be consumed as the "connect response"
		// (-> "doesn't have connect response type") and the connect sequence would fail forever.
		{ std::scoped_lock lock(inboundMutex_); while (!inboundQueue_.empty()) { inboundQueue_.pop(); } }
		{ std::scoped_lock lock(outboundMutex_); while (!outboundQueue_.empty()) { outboundQueue_.pop(); } }

		if (!initialized_ || !quicClient_->connect()) {
			settings::Logger::logError("[quic] Failed to start connection attempt");
			connectionState_.store(ConnectionState::NOT_CONNECTED);
			return;
		}

		// connect() only INITIATES the QUIC handshake; onConnected() fires later (from an msquic
		// worker thread) and flips connectionState_ CONNECTING -> CONNECTED. Block here until that
		// happens (or the connection fails / times out) so initializeConnection honours the same
		// synchronous "connected on return" contract as the MQTT channel. Without this the caller
		// runs the fleet-protocol connect sequence (send devices / read connect response) on a
		// not-yet-connected transport, which fails every cycle and leaves the connection stuck in
		// CONNECTING.
		//
		// Deliberately an untimed wait, NOT settings::receive_message_timeout (a fixed 5s constant):
		// that used to race the configured, per-endpoint quic-settings.DisconnectTimeoutMs (the value
		// msquic itself uses to bound how long it keeps retrying the handshake before firing
		// onDisconnected/shutdown). On a link whose real handshake RTT sits close to or above 5s
		// (observed on a cellular path: ~6s to first server response), the fixed 5s cap fired first,
		// tearing down an attempt msquic would otherwise have completed a moment later - and since
		// closeConnection() below also can't finish faster than the transport unwinds, every cycle
		// paid the timeout twice for no benefit. msquic guarantees a terminal onConnected/onDisconnected
		// callback within DisconnectTimeoutMs regardless, so that's the only bound this should honour -
		// waiting here without a second, disconnected deadline makes it the single source of truth.
		//
		// Update: reintroduced a bounded wait below, using settings::connect_handshake_backstop_timeout
		// instead of the old fixed 5s settings::receive_message_timeout. "msquic guarantees a terminal
		// callback" above does not hold unconditionally - this same connection has previously hung with
		// no callback ever firing (root-caused inside msquic itself, not application code), and nothing
		// else recovers this wait during normal operation (ExternalClient only cancels a stuck connect
		// attempt when the app is already shutting down). connect_handshake_backstop_timeout is set well
		// above DisconnectTimeoutMs so msquic's own callback is expected to win this race every time in
		// the common case; this bound only exists to eventually recover if it does not.
		{
			std::unique_lock lock(outboundMutex_);
			outboundCv_.wait_for(lock, settings::connect_handshake_backstop_timeout, [this] {
				return connectionState_.load() != ConnectionState::CONNECTING;
			});
		}
		if (connectionState_.load() != ConnectionState::CONNECTED) {
			settings::Logger::logError("[quic] handshake did not complete (state={})",
			                           common_utils::EnumUtils::connectionStateToString(connectionState_));
			closeConnection();

			// closeConnection() only requests the disconnect; give onDisconnected() a bounded chance
			// to arrive before giving up, so a late onConnected() isn't left racing a hard
			// NOT_CONNECTED store below (which would otherwise desync gateway state from the
			// transport -- see the CAS instead of an unconditional store).
			{
				std::unique_lock lock(outboundMutex_);
				outboundCv_.wait_for(lock, settings::receive_message_timeout, [this] {
					return connectionState_.load() == ConnectionState::NOT_CONNECTED;
				});
			}

			expected = ConnectionState::CONNECTING;
			connectionState_.compare_exchange_strong(expected, ConnectionState::NOT_CONNECTED);
		}
	}

	bool QuicCommunication::sendMessage(ExternalProtocol::ExternalClient *message) {
		if (connectionState_.load() == ConnectionState::NOT_CONNECTED) {
			settings::Logger::logWarning("[quic] Connection not established, cannot send message");
			return false;
		}

		{
			auto copy = std::make_unique<ExternalProtocol::ExternalClient>(*message);
			std::lock_guard lock(outboundMutex_);
			outboundQueue_.push(std::move(copy));
		}
		settings::Logger::logDebug("[quic] Notifying sender thread about enqueued message");
		outboundCv_.notify_one();
		return true;
	}

	/**
	 * @brief Retrieves the next message received from the server.
	 *
	 * Waits until a message is available, receive cancellation occurs, the connection
	 * enters an invalid state, or the receive timeout expires.
	 *
	 * @return The next queued server message, or nullptr if no message is available.
	 */
	std::unique_ptr<ExternalProtocol::ExternalServer> QuicCommunication::receiveMessage() {
		using enum ConnectionState;
		std::unique_lock lock(inboundMutex_);

		// Wait for a message or transition out of allowed states
		// Explicitly allow waiting during CONNECTING, CLOSING, and CONNECTED states
		// This whitelist approach is safe if new states are added in the future
		if (!inboundCv_.wait_for(
			lock,
			settings::receive_message_timeout,
			[this] {
				using enum ConnectionState;
				auto state = connectionState_.load();
				return !inboundQueue_.empty() ||
				       cancelReceive_.load() ||
				       (state != CONNECTING &&
				        state != CLOSING &&
				        state != CONNECTED);
			}
		)) {
			return nullptr;
		}

		// Check if we stopped waiting due to invalid state or empty queue
		auto state = connectionState_.load();
		if ((state != CONNECTING &&
		     state != CLOSING &&
		     state != CONNECTED) ||
		    inboundQueue_.empty()) {
			return nullptr;
		}

		auto msg = std::move(inboundQueue_.front());
		inboundQueue_.pop();
		return msg;
	}

	/**
	 * @brief Requests asynchronous disconnection from the QUIC server.
	 */
	void QuicCommunication::closeConnection() {
		if (quicClient_) {
			quicClient_->disconnect();
		}
		/// Asynchronously waiting for the transport's onDisconnected callback, then continue there
	}

	/**
	 * @brief Cancels pending receive operations.
	 *
	 * Wakes threads waiting for inbound messages so they can observe the cancellation.
	 */
	void QuicCommunication::cancelReceive() {
		{
			std::lock_guard lock(inboundMutex_);
			cancelReceive_.store(true);
		}
		inboundCv_.notify_all();
	}

	/**
	 * @brief Stops the QUIC communication channel and waits for its sender thread to finish.
	 */
	void QuicCommunication::stop() {
		if (quicClient_) {
			quicClient_->disconnect();
		}
		connectionState_.store(ConnectionState::NOT_CONNECTED);
		// Notify under the guarding mutexes so the sender thread (which waits without a timeout)
		// can't miss this NOT_CONNECTED transition and hang the destructor that joins it.
		{
			std::lock_guard lock(inboundMutex_);
			inboundCv_.notify_all();
		}
		{
			std::lock_guard lock(outboundMutex_);
			outboundCv_.notify_all();
		}
		// Explicitly stop+join here so ~QuicCommunication() never relies on senderThread_'s own
		// destructor doing this implicitly, while a late onConnected() from an msquic worker thread
		// could still be assigning to the same member -- see senderThreadMutex_. This also guarantees
		// the sender thread is fully stopped before quicClient_ is torn down by the destructor.
		{
			std::lock_guard senderLock(senderThreadMutex_);
			if (senderThread_.joinable()) {
				senderThread_.request_stop();
				senderThread_.join();
			}
		}

		settings::Logger::logInfo("[quic] Connection stopped");
	}

	/**
	 * @brief Queues a decoded server message and wakes a waiting receiver.
	 *
	 * @param msg Decoded server message to enqueue.
	 */
	void QuicCommunication::onMessageDecoded(
		std::unique_ptr<ExternalProtocol::ExternalServer> msg
	) {
		{
			std::scoped_lock lock(inboundMutex_);
			inboundQueue_.push(std::move(msg));
		}
		settings::Logger::logDebug("[quic] Notifying receiver thread about dequeued message");
		inboundCv_.notify_one();
	}

	/**
	 * @brief Serializes and sends a client message through the QUIC connection.
	 *
	 * Messages that cannot be serialized or sent are dropped and logged.
	 *
	 * @param message Client message to send.
	 */
	void QuicCommunication::sendViaQuicClient(const ExternalProtocol::ExternalClient &message) {
		std::string serialized;
		if (!message.SerializeToString(&serialized)) {
			settings::Logger::logError("[quic] Message serialization failed");
			return;
		}

		const std::span<const std::uint8_t> bytes(reinterpret_cast<const std::uint8_t *>(serialized.data()),
		                                           serialized.size());
		if (!quicClient_ || !quicClient_->send(bytes)) {
			settings::Logger::logError("[quic] Failed to send message, message dropped");
			return;
		}

		settings::Logger::logDebug("[quic] Message sent");
	}

	/**
	 * @brief Handles successful QUIC connection establishment.
	 *
	 * Transitions the connection to the connected state, starts the sender thread,
	 * and wakes operations waiting for the connection.
	 */
	void QuicCommunication::onConnected() {
		settings::Logger::logInfo("[quic] Connected to server");

		auto expected = ConnectionState::CONNECTING;
		if (connectionState_.compare_exchange_strong(expected, ConnectionState::CONNECTED)) {
			/// Start sender thread only after connection is fully established
			{
				std::lock_guard senderLock(senderThreadMutex_);
				senderThread_ = std::jthread(&QuicCommunication::senderLoop, this);
			}
			// Notify under outboundMutex_: the CONNECTED transition above must not slip between
			// initializeConnection()'s predicate check and its wait_for(), or the wakeup is lost
			// and the connect blocks for the full timeout.
			std::lock_guard lock(outboundMutex_);
			outboundCv_.notify_all();
		}
	}

	/**
	 * @brief Completes connection shutdown and releases the sender thread.
	 *
	 * Marks the connection as disconnected, wakes threads waiting for inbound or
	 * outbound activity, and stops and joins the sender thread.
	 */
	void QuicCommunication::onDisconnected() {
		settings::Logger::logInfo("[quic] Connection shutdown complete");

		connectionState_.store(ConnectionState::NOT_CONNECTED);
		// Notify each condition variable while holding the mutex that guards its wait predicate,
		// so the NOT_CONNECTED transition can't be missed between a waiter's predicate check and
		// its wait() (a lost wakeup would hang the sender thread, which waits without a timeout).
		{
			std::lock_guard lock(outboundMutex_);
			outboundCv_.notify_all();
		}
		{
			std::lock_guard lock(inboundMutex_);
			inboundCv_.notify_all();
		}
		// Explicitly stop+join the sender thread now rather than leaving it to the next
		// onConnected()'s move-assign (or ~QuicCommunication()'s implicit destruction). The state
		// change above already woke senderLoop's untimed wait, so this join is bounded -- it does not
		// block on an in-flight quicClient_->send(). Guarded by senderThreadMutex_ against a
		// concurrent onConnected() assigning a new thread from another msquic worker callback.
		{
			std::lock_guard senderLock(senderThreadMutex_);
			if (senderThread_.joinable()) {
				senderThread_.request_stop();
				senderThread_.join();
			}
		}
	}

	/**
	 * @brief Handles a connection shutdown initiated by the peer.
	 *
	 * Marks the connection as closing and wakes the outbound sender loop.
	 */
	void QuicCommunication::onShutdownInitiatedByPeer() {
		settings::Logger::logWarning("[quic] Connection shutdown initiated by peer");
		connectionState_.store(ConnectionState::CLOSING);
		// Wake senderLoop's untimed wait: its predicate already treats CLOSING as "stop looping", but
		// it only re-checks the predicate when notified, so without this it stays blocked until
		// onDisconnected() arrives instead of unwinding as soon as the peer starts closing.
		std::lock_guard lock(outboundMutex_);
		outboundCv_.notify_all();
	}

	/**
	 * @brief Processes bytes received from the QUIC connection as an external server message.
	 *
	 * @param bytes Serialized external server message data.
	 */
	void QuicCommunication::onBytesReceived(std::vector<std::uint8_t> bytes) {
		auto msg = std::make_unique<ExternalProtocol::ExternalServer>();
		if (!msg->ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) {
			settings::Logger::logError("[quic] Failed to parse ExternalServer message");
			return;
		}
		onMessageDecoded(std::move(msg));
	}

	/**
	 * @brief Sends queued client messages while the QUIC connection is active.
	 */
	void QuicCommunication::senderLoop() {
		settings::Logger::logDebug("[quic] Sender thread loop started");

		while (connectionState_.load() == ConnectionState::CONNECTED) {
			std::unique_ptr<ExternalProtocol::ExternalClient> msg;

			{
				std::unique_lock lock(outboundMutex_);

				settings::Logger::logDebug("[quic] Sender thread loop waiting for outbound queue");
				outboundCv_.wait(lock, [this] {
					return !outboundQueue_.empty() ||
					       connectionState_.load() != ConnectionState::CONNECTED;
				});

				if (connectionState_.load() != ConnectionState::CONNECTED) {
					break;
				}

				settings::Logger::logDebug("[quic] Sender thread loop sending outbound queue");
				msg = std::move(outboundQueue_.front());
				outboundQueue_.pop();
			}

			sendViaQuicClient(*msg);
		}
	}

	/**
	 * @brief Retrieves a protocol setting and converts JSON string values to plain strings.
	 *
	 * @param settings Connection settings containing the protocol setting.
	 * @param key Name of the protocol setting to retrieve.
	 * @param defaultValue Value returned when the setting is missing or contains invalid JSON.
	 * @return The setting value, with JSON string values unwrapped, or `defaultValue` when unavailable or invalid.
	 */
	std::string QuicCommunication::getProtocolSettingsString(
		const structures::ExternalConnectionSettings &settings,
		std::string_view key,
		std::string defaultValue
	) {
		const auto it = settings.protocolSettings.find(key);
		if (it == settings.protocolSettings.end()) {
			settings::Logger::logWarning("[quic] Protocol setting '{}' not found, using default", key);
			return defaultValue;
		}

		const auto &raw = it->second;

		try {
			if (nlohmann::json::accept(raw)) {
				auto j = nlohmann::json::parse(raw);
				if (j.is_string()) {
					return j.get<std::string>();
				}
			}
			return raw;
		} catch (const nlohmann::json::exception &) {
			settings::Logger::logWarning("[quic] Protocol setting '{}' contains invalid JSON, using default", key);
			return defaultValue;
		}
	}
}
