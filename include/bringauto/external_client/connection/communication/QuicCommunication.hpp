#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/external_client/connection/ConnectionState.hpp>

#include <bringauto/quic/QuicClient.hpp>

#include <nlohmann/json.hpp>

#include <condition_variable>
#include <queue>
#include <thread>


namespace bringauto::external_client::connection::communication {
	class QuicCommunication : public ICommunicationChannel {
	public:
		explicit QuicCommunication(const structures::ExternalConnectionSettings &settings, const std::string &company,
		                           const std::string &vehicleName);

		~QuicCommunication() override;

		/**
		 * @brief Initializes a QUIC connection to the server.
		 *
		 * Attempts to establish a new QUIC connection.
		 * It first atomically verifies that the current connection state is
		 * NOT_CONNECTED and transitions it to CONNECTING in order to prevent
		 * concurrent connection attempts. Any messages left over from a previous
		 * session are dropped so a stale frame can't be consumed as this
		 * session's connect response.
		 *
		 * After the state transition, it starts the underlying QUIC client
		 * connection attempt and blocks until the handshake completes (or times
		 * out), so callers observe the same synchronous "connected on return"
		 * contract as the MQTT channel.
		 *
		 * Any failures are logged.
		 */
		void initializeConnection() override;

		/**
		 * @brief Enqueues an outgoing message to be sent over the QUIC connection.
		 *
		 * Creates a shared copy of the provided ExternalClient message
		 * and pushes it into the outbound message queue in a thread-safe manner.
		 * After enqueuing, it notifies the sender thread via a condition variable
		 * that a new message is available for sending.
		 *
		 * @param message Pointer to the message that should be sent.
		 * @return true Always returns true to indicate the message was successfully enqueued.
		 */
		bool sendMessage(ExternalProtocol::ExternalClient *message) override;

		/**
		 * @brief Receives an incoming message from the QUIC connection.
		 *
		 * Waits for an incoming message to appear in the inbound
		 * queue or for the connection state to change from CONNECTED.
		 * The wait is bounded by a configurable timeout.
		 *
		 * If the wait times out, the connection is no longer in the CONNECTED
		 * state, or no message is available, the function returns nullptr.
		 * Otherwise, it retrieves and removes the next message from the inbound
		 * queue and returns it.
		 *
		 * @return A unique pointer to the received ExternalServer message,
		 *         or nullptr if no message is available or the connection is not active.
		 */
		std::unique_ptr<ExternalProtocol::ExternalServer> receiveMessage() override;

		/**
		 * @brief Initiates a graceful shutdown of the QUIC connection.
		 *
		 * Requests an orderly shutdown of the active QUIC connection.
		 * If no connection is currently established, the function returns immediately.
		 *
		 * The shutdown is performed asynchronously. Completion is signaled via the
		 * transport's onDisconnected callback.
		 */
		void closeConnection() override;

		void cancelReceive() override;

		bool consumeServerDisconnectNotification() override { return false; }

	private:
		/// Atomic state of the connection used for synchronization across threads
		std::atomic<ConnectionState> connectionState_{ConnectionState::NOT_CONNECTED};

		/// Set by cancelReceive() to unblock a pending receiveMessage() call immediately
		std::atomic<bool> cancelReceive_{false};

		/// @name Inbound (peer → this)
		/// @{
		/// Queue of incoming messages received from the peer
		std::queue<std::unique_ptr<ExternalProtocol::ExternalServer> > inboundQueue_;
		/// Mutex protecting access to the inbound message queue
		std::mutex inboundMutex_;
		/// Condition variable for signaling inbound message availability
		std::condition_variable inboundCv_;
		/// @}

		/// @name Outbound (this → peer)
		/// @{
		/// Queue of outgoing messages to be sent to the peer
		std::queue<std::unique_ptr<ExternalProtocol::ExternalClient> > outboundQueue_;
		/// Mutex protecting access to the outbound message queue
		std::mutex outboundMutex_;
		/// Condition variable for signaling outbound message availability. Also used to wake
		/// initializeConnection()'s wait for the handshake to complete -- onConnected()/
		/// onDisconnected() both notify it.
		std::condition_variable outboundCv_;
		/// @}

		/// Shared, transport-only QUIC client. Owns the msquic registration/connection
		/// lifecycle, credential setup, and per-message unidirectional stream framing + reassembly.
		///
		/// Declared after connectionState_/cancelReceive_/the queues+condvars: msquic's
		/// RegistrationClose (invoked from ~QuicClient(), which runs as this member is destroyed)
		/// blocks until every in-flight callback for this client has fully returned, and one such
		/// callback can still be onDisconnected() racing in on an msquic worker thread concurrently
		/// with our own destructor. Because those other members are declared (and therefore
		/// destroyed) *after* quicClient_, they are still alive for the whole time quicClient_'s
		/// destructor can possibly touch them.
		std::unique_ptr<bringauto::quic::QuicClient> quicClient_;

		/// True once quicClient_ was constructed and quicClient_->initialize() succeeded. When false,
		/// initializeConnection() is a no-op (invalid config or transport init failure was already
		/// logged during construction).
		bool initialized_{false};

		/// Dedicated sender thread responsible for transmitting outbound messages. Needed because
		/// ExternalConnection enqueues the Fleet-protocol Connect message immediately after
		/// initializeConnection() returns, before the QUIC handshake completes -- this thread only
		/// starts draining the queue once the transport reports CONNECTED. Declared after
		/// quicClient_ so it (and its last use of quicClient_) is joined before quicClient_ is
		/// destroyed.
		std::jthread senderThread_;

		/**
		 * @brief Builds the shared client's endpoint configuration (host/port/ALPN/credentials)
		 * from this connection's settings.
		 */
		static bringauto::quic::QuicEndpointConfig buildEndpointConfig(const structures::ExternalConnectionSettings &settings);

		/**
		 * @brief Builds the shared client's transport settings from the "quic-settings" protocol
		 * settings. Unrecognized keys (e.g. a leftover "stream-mode" from before this migration) are
		 * logged as warnings rather than silently ignored.
		 */
		static bringauto::quic::QuicSettings buildQuicSettings(const structures::ExternalConnectionSettings &settings);

		/**
		 * @brief Fired once the QUIC handshake completes. Transitions CONNECTING -> CONNECTED and
		 * starts the sender thread.
		 */
		void onConnected();

		/**
		 * @brief Fired once a terminal disconnect is reported (graceful drop or a connect attempt
		 * that never completed). Transitions to NOT_CONNECTED and wakes both queues.
		 */
		void onDisconnected();

		/**
		 * @brief Fired when the peer initiates a graceful shutdown. Transitions to CLOSING so a
		 * pending receiveMessage() keeps waiting for the final onDisconnected() instead of erroring
		 * out immediately.
		 */
		void onShutdownInitiatedByPeer();

		/**
		 * @brief Fired once per fully-reassembled inbound frame. Parses it as an ExternalServer
		 * message and enqueues it, mirroring the old streamCallback's RECEIVE case.
		 */
		void onBytesReceived(std::vector<std::uint8_t> bytes);

		/**
		 * @brief Handles a successfully decoded incoming message.
		 *
		 * Pushes the decoded ExternalServer message into the inbound queue
		 * in a thread-safe manner and notifies the receiver thread that a
		 * new message is available.
		 *
		 * @param msg Decoded message received from the peer.
		 */
		void onMessageDecoded(std::unique_ptr<ExternalProtocol::ExternalServer> msg);

		/**
		 * @brief Sends a message to the peer via the shared QUIC client.
		 *
		 * @param message Message to be sent to the peer.
		 */
		void sendViaQuicClient(const ExternalProtocol::ExternalClient &message);

		/**
		 * @brief Stops the QUIC communication and releases all resources.
		 *
		 * Requests connection shutdown and unblocks any waiting sender and
		 * receiver threads so the sender thread can join cleanly before
		 * quicClient_ itself is destroyed.
		 */
		void stop();

		/**
		 * @brief Sender thread main loop for outbound messages.
		 *
		 * Waits for outbound messages while the connection is in the CONNECTED state.
		 * Messages are dequeued and sent over individual QUIC streams.
		 *
		 * The loop terminates when the connection leaves the CONNECTED state.
		 */
		void senderLoop();

		/**
		 * @brief Retrieves a protocol setting value as a plain string.
		 *
		 * Looks up a value in ExternalConnectionSettings::protocolSettings and returns
		 * it as a plain string. If the stored value is a JSON-encoded string, it is
		 * transparently parsed and unwrapped.
		 *
		 * If the key does not exist or the value cannot be parsed as valid JSON,
		 * the provided default value is returned and a warning is logged.
		 *
		 * This allows uniform access to protocol settings regardless of whether
		 * they are stored as plain strings or JSON-serialized strings.
		 *
		 * @param settings External connection settings containing protocolSettings.
		 * @param key Key identifying the protocol setting.
		 * @param defaultValue Value returned if the key is missing or invalid.
		 * @return Plain string value suitable for direct use (e.g. file paths).
		 */
		static std::string getProtocolSettingsString(
			const structures::ExternalConnectionSettings &settings,
			std::string_view key,
			std::string defaultValue = {}
		);
	};
}
