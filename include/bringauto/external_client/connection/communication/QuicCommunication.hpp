#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/external_client/connection/ConnectionState.hpp>

#include <msquic.h>
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
		 * concurrent connection attempts.
		 *
		 * After the state transition, it opens a QUIC connection handle and
		 * starts the connection using the configured server address, port,
		 * and QUIC configuration.
		 *
		 * Any failures during the connection open or start process are logged.
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
		 * @return A shared pointer to the received ExternalServer message,
		 *         or nullptr if no message is available or the connection is not active.
		 */
		std::shared_ptr<ExternalProtocol::ExternalServer> receiveMessage() override;

		/**
		 * @brief Initiates a graceful shutdown of the QUIC connection.
		 *
		 * Requests an orderly shutdown of the active QUIC connection.
		 * If no connection is currently established, the function returns immediately.
		 *
		 * The shutdown is performed asynchronously. Completion is signaled via the
		 * QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE event, which is handled in
		 * the connectionCallback.
		 */
		void closeConnection() override;

	private:
		/// Directionality of QUIC streams created or accepted by this connection
		enum class StreamMode {
			/// Stream can only send data in one direction
			Unidirectional,
			/// Stream supports bidirectional send and receive
			Bidirectional
		};

		/// Pointer to the MsQuic API function table
		const QUIC_API_TABLE *quic_{nullptr};
		/// QUIC registration handle associated with the application
		HQUIC registration_{nullptr};
		/// QUIC configuration handle (ALPN, credentials, transport settings)
		HQUIC config_{nullptr};
		/// Active QUIC connection handle
		HQUIC connection_{nullptr};
		/// Application-Layer Protocol Negotiation (ALPN) string
		std::string alpn_;
		/// QUIC buffer wrapping the ALPN string
		QUIC_BUFFER alpnBuffer_{};

		/// Path to the client certificate file
		std::string certFile_;
		/// Path to the client private key file
		std::string keyFile_;
		/// Path to the CA certificate file
		std::string caFile_;

		StreamMode streamMode_{StreamMode::Bidirectional};

		/// Atomic state of the connection used for synchronization across threads
		std::atomic<ConnectionState> connectionState_{ConnectionState::NOT_CONNECTED};

		/// @name Inbound (peer → this)
		/// @{
		/// Queue of incoming messages received from the peer
		std::queue<std::shared_ptr<ExternalProtocol::ExternalServer> > inboundQueue_;
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
		/// Condition variable for signaling outbound message availability
		std::condition_variable outboundCv_;
		/// Dedicated sender thread responsible for transmitting outbound messages
		std::jthread senderThread_;
		/// @}

		/**
		 * @brief Owns memory for a single MsQuic StreamSend operation.
		 *
		 * SendBuffer wraps a QUIC_BUFFER together with its backing storage.
		 * The memory must remain valid until MsQuic signals
		 * QUIC_STREAM_EVENT_SEND_COMPLETE, at which point it can be safely freed.
		 *
		 * Instances of this struct are typically allocated on the heap and passed
		 * to MsQuic via the StreamSend ClientContext pointer.
		 */
		struct SendBuffer {
			QUIC_BUFFER buffer{};
			std::string storage;

			/**
			 * @brief Constructs a SendBuffer with zero-initialized storage.
			 *
			 * Allocates storage of the given size, fills it with zero bytes,
			 * and initializes the QUIC_BUFFER to point to this storage.
			 *
			 * @param size Number of bytes to allocate for the send buffer.
			 */
			explicit SendBuffer(size_t size)
				: storage(size, '\0') {
				buffer.Length = static_cast<uint32_t>(storage.size());
				buffer.Buffer = reinterpret_cast<uint8_t *>(storage.data());
			}
		};

		/**
		 * @brief Loads and initializes the MsQuic API.
		 *
		 * Initializes the MsQuic library and retrieves the
		 * QUIC API function table. The resulting table is stored for later
		 * use when creating registrations, configurations, and connections.
		 *
		 * If the initialization fails, an error is logged.
		 */
		void loadMsQuic();

		/**
		 * @brief Initializes a QUIC registration.
		 *
		 * Creates a QUIC registration with the specified application name and
		 * a low-latency execution profile. The registration is required for
		 * creating QUIC configurations and connections.
		 *
		 * If registration creation fails, an error is logged.
		 */
		void initRegistration();

		/**
		 * @brief Initializes the QUIC configuration and loads client credentials.
		 *
		 * Opens a QUIC configuration using the configured ALPN and default QUIC
		 * transport settings. Client TLS credentials are then set up using a
		 * certificate file, private key file, and CA certificate file.
		 *
		 * If configuration creation or credential loading fails, an error is logged.
		 */
		void initConfiguration();

		/**
		 * @brief Opens a QUIC configuration.
		 *
		 * Creates a QUIC configuration associated with the current registration,
		 * configured ALPN, and optional transport settings.
		 *
		 * If settings are not provided, default QUIC settings are used.
		 * On failure, an error is logged.
		 */
		void configurationOpen();

		/**
		 * @brief Loads TLS credentials into the QUIC configuration.
		 *
		 * Loads client-side TLS credentials into the active QUIC configuration.
		 * The credentials define the certificate, private key, and CA certificate
		 * used for secure communication.
		 *
		 * If credential loading fails, an error is logged.
		 *
		 * @param credential Pointer to the QUIC credential configuration.
		 */
		void configurationLoadCredential(const QUIC_CREDENTIAL_CONFIG *credential) const;

		/**
		 * @brief Handles a successfully decoded incoming message.
		 *
		 * Pushes the decoded ExternalServer message into the inbound queue
		 * in a thread-safe manner and notifies the receiver thread that a
		 * new message is available.
		 *
		 * @param msg Decoded message received from the peer.
		 */
		void onMessageDecoded(std::shared_ptr<ExternalProtocol::ExternalServer> msg);

		/**
		 * @brief Sends a message to the peer using a QUIC stream.
		 *
		 * Opens a new QUIC stream on the active connection and serializes the
		 * provided ExternalClient message into a byte buffer.
		 * The message is sent using a single StreamSend call with START and FIN
		 * flags, effectively opening, sending, and closing the stream.
		 *
		 * The allocated send buffer is released asynchronously in the
		 * QUIC_STREAM_EVENT_SEND_COMPLETE callback.
		 *
		 * @param message Message to be sent to the peer.
		 */
		void sendViaQuicStream(const ExternalProtocol::ExternalClient &message);

		/**
		 * @brief Closes the active QUIC configuration.
		 */
		void closeConfiguration();

		/**
		 * @brief Closes the QUIC registration.
		 */
		void closeRegistration();

		/**
		 * @brief Closes the MsQuic API and releases associated resources.
		 */
		void closeMsQuic();

		/**
		 * @brief Stops the QUIC communication and releases all resources.
		 *
		 * Initiates connection shutdown and closes the QUIC configuration,
		 * registration, and MsQuic API in the correct order.
		 * All waiting sender and receiver threads are unblocked by notifying
		 * the associated condition variables.
		 */
		void stop();

		/**
		 * @brief Handles QUIC connection-level events.
		 *
		 * Processes connection lifecycle events reported by MsQuic, including
		 * successful connection establishment, peer-initiated shutdown, and
		 * shutdown completion.
		 *
		 * All QUIC_CONNECTION_EVENT cases are documented at
		 * https://microsoft.github.io/msquic/msquicdocs/docs/api/QUIC_CONNECTION_EVENT.html
		 *
		 * @param connection QUIC connection handle.
		 * @param context User-defined context pointer (QuicCommunication instance).
		 * @param event Connection event information provided by MsQuic.
		 * @return QUIC_STATUS_SUCCESS to indicate successful event handling.
		 */
		static QUIC_STATUS QUIC_API connectionCallback(HQUIC connection, void *context, QUIC_CONNECTION_EVENT *event);

		/**
		 * @brief Handles QUIC stream-level events.
		 *
		 * Processes stream events reported by MsQuic, including data reception,
		 * send completion, stream startup, and shutdown notifications.
		 *
		 * All QUIC_STREAM_EVENT cases are documented at
		 * https://microsoft.github.io/msquic/msquicdocs/docs/api/QUIC_STREAM_EVENT.html
		 *
		 * @param stream QUIC stream handle associated with the event.
		 * @param context User-defined context pointer (QuicCommunication instance).
		 * @param event Stream event information provided by MsQuic.
		 * @return QUIC_STATUS_SUCCESS to indicate successful event handling.
		 */
		static QUIC_STATUS QUIC_API streamCallback(HQUIC stream, void *context, QUIC_STREAM_EVENT *event);

		/**
		 * @brief Sender thread main loop for outbound messages.
		 *
		 * Waits for outbound messages while the connection is in the CONNECTED state.
		 * Messages are dequeued and sent over individual QUIC streams.
		 *
		 * If sending fails, the message is re-enqueued for a later retry.
		 * The loop terminates when the connection leaves the CONNECTED state.
		 */
		void senderLoop();

		/**
		 * @brief Retrieves the QUIC stream identifier for the given stream handle.
		 *
		 * Queries MsQuic for the stream ID associated with the provided HQUIC stream.
		 * If the parameter query fails, an empty optional is returned.
		 *
		 * @param stream Valid QUIC stream handle.
		 * @return Stream identifier on success, or std::nullopt if the query fails.
		 */
		std::optional<uint64_t> getStreamId(HQUIC stream);

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

		/**
		 * @brief Parses QUIC stream mode from protocol settings.
		 *
		 * Reads the stream mode from the external connection settings and determines
		 * whether QUIC streams should be unidirectional or bidirectional.
		 *
		 * Supported values:
		 *  - "unidirectional", "unidir" → Unidirectional streams
		 *  - any other value or missing setting → Bidirectional streams (default)
		 *
		 * @param settings External connection settings containing QUIC protocol options
		 * @return StreamMode Parsed stream mode (defaults to Bidirectional)
		 */
		static StreamMode parseStreamMode(const structures::ExternalConnectionSettings &settings);
	};
}
