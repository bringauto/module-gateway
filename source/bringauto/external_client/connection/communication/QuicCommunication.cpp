#include <bringauto/common_utils/EnumUtils.hpp>
#include <bringauto/external_client/connection/communication/QuicCommunication.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/settings/LoggerId.hpp>


namespace bringauto::external_client::connection::communication {
	QuicCommunication::QuicCommunication(const structures::ExternalConnectionSettings &settings,
	                                     const std::string &company,
	                                     const std::string &vehicleName) : ICommunicationChannel(settings),
	                                                                       alpn_(getProtocolSettingsString(
		                                                                       settings,
		                                                                       settings::Constants::ALPN)),
	                                                                       certFile_(getProtocolSettingsString(
		                                                                       settings,
		                                                                       settings::Constants::CLIENT_CERT)),
	                                                                       keyFile_(getProtocolSettingsString(
		                                                                       settings,
		                                                                       settings::Constants::CLIENT_KEY)),
	                                                                       caFile_(getProtocolSettingsString(
		                                                                       settings,
		                                                                       settings::Constants::CA_FILE)) {
		alpnBuffer_.Buffer = reinterpret_cast<uint8_t *>(alpn_.data());
		alpnBuffer_.Length = static_cast<uint32_t>(alpn_.size());

		settings::Logger::logInfo("[quic] Initialize QUIC communication to {}:{} for {}/{}", settings.serverIp,
		                          settings.port, company, vehicleName);

		try {
			loadMsQuic();
			initRegistration("module-gateway-quic-client");
			initConfiguration();
		} catch (const std::exception &e) {
			settings::Logger::logError("[quic] Failed to initialize QUIC communication; {}", e.what());
		}
	}

	QuicCommunication::~QuicCommunication() {
		stop();
	}

	void QuicCommunication::initializeConnection() {
		settings::Logger::logDebug("[quic] Connecting to server when {}",
		                           common_utils::EnumUtils::connectionStateToString(connectionState_));

		ConnectionState expected = ConnectionState::NOT_CONNECTED;
		if (!connectionState_.
			compare_exchange_strong(expected, ConnectionState::CONNECTING, std::memory_order_acq_rel)) {
			settings::Logger::logError("Connection already in progress or established");
			return;
		}

		QUIC_STATUS status = quic_->ConnectionOpen(registration_, connectionCallback, this, &connection_);
		if (QUIC_FAILED(status)) {
			settings::Logger::logError("ConnectionOpen failed (status=0x{:x})", status);
			return;
		}

		status = quic_->ConnectionStart(
			connection_,
			config_,
			QUIC_ADDRESS_FAMILY_INET,
			settings_.serverIp.c_str(),
			settings_.port
		);

		if (QUIC_FAILED(status)) {
			settings::Logger::logError("ConnectionOpen failed (status=0x{:x})", status);
			return;
		}

		connectionState_ = ConnectionState::CONNECTING;
	}

	bool QuicCommunication::sendMessage(ExternalProtocol::ExternalClient *message) {
		if (connectionState_.load() == ConnectionState::NOT_CONNECTED) {
			settings::Logger::logWarning("[quic] Connection not established, cannot send message");
			return false;
		}

		{
			auto copy = std::make_shared<ExternalProtocol::ExternalClient>(*message);
			std::lock_guard<std::mutex> lock(outboundMutex_);
			outboundQueue_.push(std::move(copy));
		}
		settings::Logger::logDebug("[quic] Notifying sender thread about enqueued message");
		outboundCv_.notify_one();
		return true;
	}

	std::shared_ptr<ExternalProtocol::ExternalServer> QuicCommunication::receiveMessage() {
		std::unique_lock<std::mutex> lock(inboundMutex_);

		if (!inboundCv_.wait_for(
			lock,
			settings::receive_message_timeout,
			[this] { return !inboundQueue_.empty() || connectionState_.load() != ConnectionState::CONNECTED; }
		)) {
			return nullptr;
		}

		if (connectionState_.load() != ConnectionState::CONNECTED || inboundQueue_.empty()) {
			return nullptr;
		}

		auto msg = inboundQueue_.front();
		inboundQueue_.pop();
		return msg;
	}

	void QuicCommunication::loadMsQuic() {
		QUIC_STATUS status = MsQuicOpen2(&quic_);
		if (QUIC_FAILED(status)) {
			settings::Logger::logError("[quic] Failed to open QUIC; QUIC_STATUS => {:x}", status);
			return;
		}
	}

	void QuicCommunication::initRegistration(std::string appName) {
		QUIC_REGISTRATION_CONFIG config{};
		config.AppName = appName.c_str();
		config.ExecutionProfile = QUIC_EXECUTION_PROFILE_LOW_LATENCY;

		QUIC_STATUS status = quic_->RegistrationOpen(&config, &registration_);
		if (QUIC_FAILED(status)) {
			settings::Logger::logError("[quic] Failed to open QUIC registration; QUIC_STATUS => {:x}", status);
			return;
		}
	}

	void QuicCommunication::initConfiguration() {
		configurationOpen(nullptr);

		QUIC_CERTIFICATE_FILE certificate{};
		certificate.CertificateFile = certFile_.c_str();
		certificate.PrivateKeyFile = keyFile_.c_str();

		QUIC_CREDENTIAL_CONFIG credential{};
		credential.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
		credential.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_SET_CA_CERTIFICATE_FILE;
		credential.CertificateFile = &certificate;
		credential.CaCertificateFile = caFile_.c_str();

		configurationLoadCredential(&credential);
	}

	void QuicCommunication::configurationOpen(const QUIC_SETTINGS *settings) {
		const uint32_t settingsSize = settings ? sizeof(*settings) : 0;

		QUIC_STATUS status = quic_->ConfigurationOpen(
			registration_,
			&alpnBuffer_,
			1,
			settings,
			settingsSize,
			nullptr,
			&config_
		);

		if (QUIC_FAILED(status)) {
			settings::Logger::logError("[quic] Failed to open QUIC configuration; QUIC_STATUS => {:x}", status);
			return;
		}
	}

	void QuicCommunication::configurationLoadCredential(const QUIC_CREDENTIAL_CONFIG *credential) const {
		const QUIC_STATUS status = quic_->ConfigurationLoadCredential(config_, credential);
		if (QUIC_FAILED(status)) {
			settings::Logger::logError("[quic] Failed to load QUIC credential; QUIC_STATUS => {:x}", status);
			return;
		}
	}

	void QuicCommunication::closeConnection() {
		if (!connection_) {
			return;
		}

		quic_->ConnectionShutdown(connection_, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);

		/// Asynchronously waiting for QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE event, then continue in connectionCallback
	}

	void QuicCommunication::closeConfiguration() {
		if (config_) {
			quic_->ConfigurationClose(config_);
		}

		config_ = nullptr;
	}

	void QuicCommunication::closeRegistration() {
		if (registration_) {
			quic_->RegistrationClose(registration_);
		}

		registration_ = nullptr;
	}

	void QuicCommunication::closeMsQuic() {
		if (quic_) {
			MsQuicClose(quic_);
		}

		quic_ = nullptr;
	}

	void QuicCommunication::stop() {
		closeConnection();
		closeConfiguration();
		closeRegistration();
		closeMsQuic();

		inboundCv_.notify_all();
		outboundCv_.notify_all();

		settings::Logger::logInfo("[quic] Connection stopped");
	}

	void QuicCommunication::onMessageDecoded(
		std::shared_ptr<ExternalProtocol::ExternalServer> msg
	) {
		{
			std::lock_guard<std::mutex> lock(inboundMutex_);
			inboundQueue_.push(std::move(msg));
		}
		settings::Logger::logDebug("[quic] Notifying receiver thread about dequeued message");
		inboundCv_.notify_one();
	}

	QuicCommunication::SendBuffer::SendBuffer(size_t size) : storage(std::make_unique<uint8_t[]>(size)) {
		buffer.Length = static_cast<uint32_t>(size);
		buffer.Buffer = storage.get();
	}

	void QuicCommunication::sendViaQuicStream(const std::shared_ptr<ExternalProtocol::ExternalClient> &message) {
		HQUIC stream{nullptr};
		if (QUIC_FAILED(quic_->StreamOpen(connection_, QUIC_STREAM_OPEN_FLAG_NONE, streamCallback, this, &stream))) {
			settings::Logger::logError("[quic] StreamOpen failed");
			return;
		}

		const size_t size = message->ByteSizeLong();
		auto sendBuffer = std::make_unique<SendBuffer>(size);

		if (!message->SerializeToArray(sendBuffer->storage.get(), static_cast<int>(size))) {
			settings::Logger::logError("[quic] Message serialization failed");
			return;
		}

		settings::Logger::logDebug(
			"[quic][debug] SendBuffer ptr={}, QUIC_BUFFER ptr={}, data ptr={}, size={}",
			static_cast<void *>(sendBuffer.get()),
			static_cast<void *>(&sendBuffer->buffer),
			static_cast<void *>(sendBuffer->buffer.Buffer),
			sendBuffer->buffer.Length
		);

		const SendBuffer *raw = sendBuffer.get();
		const QUIC_BUFFER *quicBuf = &raw->buffer;
		SendBuffer *quicBufContext = sendBuffer.release();

		const QUIC_STATUS status = quic_->StreamSend(
			stream,
			quicBuf,
			1,
			/**
			 * START => Simulates quic_->StreamStart before send
			 * FIN => Simulates quic_->StreamShutdown after send
			 */
			QUIC_SEND_FLAG_START | QUIC_SEND_FLAG_FIN,
			quicBufContext
		);

		if (QUIC_FAILED(status)) {
			std::unique_ptr<SendBuffer> reclaim{quicBufContext};

			quic_->StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
			settings::Logger::logError("[quic] Failed to send QUIC stream; QUIC_STATUS => {:x}", status);
			return;
		}

		auto streamId = getStreamId(stream);
		settings::Logger::logDebug("[quic] [stream {}] Message sent", streamId ? *streamId : 0);
	}

	QUIC_STATUS QUIC_API QuicCommunication::connectionCallback(HQUIC connection, void *context,
	                                                           QUIC_CONNECTION_EVENT *event) {
		auto *self = static_cast<QuicCommunication *>(context);

		switch (event->Type) {
			/// Fired when the QUIC handshake is complete and the connection is ready
			/// for stream creation and data transfer.
			case QUIC_CONNECTION_EVENT_CONNECTED: {
				settings::Logger::logInfo("[quic] Connected to server");

				auto expected = ConnectionState::CONNECTING;
				if (self->connectionState_.compare_exchange_strong(expected, ConnectionState::CONNECTED)) {
					/// Start sender thread only after connection is fully established
					self->senderThread_ = std::jthread(&QuicCommunication::senderLoop, self);
					self->outboundCv_.notify_all();
				}
				break;
			}

			/// Final notification that the connection has been fully shut down.
			/// This is the last event delivered for the connection handle.
			case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
				settings::Logger::logInfo("[quic] Connection shutdown complete");

				self->connectionState_ = ConnectionState::NOT_CONNECTED;
				self->outboundCv_.notify_all();

				if (self->senderThread_.joinable()) {
					self->senderThread_.request_stop();
				}

				self->quic_->ConnectionClose(connection);
				self->connection_ = nullptr;
				break;
			}

			/// Peer or transport initiated connection shutdown (error or graceful close).
			/// Further sends may fail after this event.
			case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER: {
				settings::Logger::logWarning("[quic] Connection shutdown initiated by peer");
				self->connectionState_ = ConnectionState::CLOSING;
				break;
			}

			default: {
				settings::Logger::logDebug("[quic] Unhandled connection event 0x{:x}",
				                           static_cast<unsigned>(event->Type));
				break;
			}
		}

		return QUIC_STATUS_SUCCESS;
	}

	QUIC_STATUS QUIC_API QuicCommunication::streamCallback(HQUIC stream, void *context, QUIC_STREAM_EVENT *event) {
		auto *self = static_cast<QuicCommunication *>(context);
		auto streamId = self->getStreamId(stream);

		switch (event->Type) {
			/// Raised when the peer sends stream data and MsQuic delivers received bytes to the application.
			case QUIC_STREAM_EVENT_RECEIVE: {
				settings::Logger::logDebug(
					"[quic] [stream {}] Received {:d} bytes in {:d} buffers",
					streamId ? *streamId : 0,
					event->RECEIVE.TotalBufferLength,
					event->RECEIVE.BufferCount
				);

				std::vector<uint8_t> data;
				data.reserve(event->RECEIVE.TotalBufferLength);

				for (uint32_t i = 0; i < event->RECEIVE.BufferCount; ++i) {
					const auto &b = event->RECEIVE.Buffers[i];
					data.insert(data.end(), b.Buffer, b.Buffer + b.Length);
				}

				auto msg = std::make_shared<ExternalProtocol::ExternalServer>();
				if (!msg->ParseFromArray(data.data(), static_cast<int>(data.size()))) {
					settings::Logger::logError("[quic] Failed to parse ExternalServer message");
				} else {
					self->onMessageDecoded(std::move(msg));
				}

				break;
			}

			/// Raised when the peer has finished sending on this stream
			/// (peer's FIN has been fully received and processed).
			case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN: {
				settings::Logger::logDebug("[quic] [stream {}] Peer stream send shutdown", streamId ? *streamId : 0);
				break;
			}

			/// Raised when the local send direction is fully shut down
			/// and the peer has acknowledged the FIN.
			case QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE: {
				settings::Logger::logDebug("[quic] [stream {}] Stream send shutdown complete",
				                           streamId ? *streamId : 0);
				break;
			}

			/// Raised after StreamStart completes successfully
			/// and the stream becomes active with a valid stream ID.
			case QUIC_STREAM_EVENT_START_COMPLETE: {
				settings::Logger::logDebug("[quic] [stream {}] Stream start completed", streamId ? *streamId : 0);
				break;
			}

			/// Raised when a single StreamSend operation completes
			/// (data was accepted, acknowledged, or the send was canceled).
			case QUIC_STREAM_EVENT_SEND_COMPLETE: {
				/**
				 * This event is raised when MsQuic has finished processing
				 * a single StreamSend request.
				 *
				 * Meaning:
				 *  - MsQuic no longer needs the application-provided buffer:
				 *      - the data has been acknowledged (ACKed) by the peer
				 *        at the QUIC transport level and will not be retransmitted
				 *      - OR the send was canceled (Canceled == TRUE), e.g. due to
				 *        stream or connection shutdown
				 *
				 * Reliability semantics:
				 *  - the ACK is strictly a QUIC transport-level acknowledgment
				 *  - it does NOT mean the peer application has read or processed
				 *    the data
				 *
				 * Practical consequence:
				 *  - this is the only correct place to safely free the memory
				 *    passed to StreamSend (via ClientContext)
				 */
				if (event->SEND_COMPLETE.Canceled) {
					settings::Logger::logDebug("[quic] [stream {}] Stream send canceled",
					                           streamId ? *streamId : 0);
				} else {
					settings::Logger::logDebug("[quic] [stream {}] Stream send completed",
					                           streamId ? *streamId : 0);
				}

				std::unique_ptr<SendBuffer> sendBuf{
					static_cast<SendBuffer *>(event->SEND_COMPLETE.ClientContext)
				};

				break;
			}

			/// Raised when both send and receive directions are closed
			/// and the stream lifecycle is fully complete.
			case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE: {
				settings::Logger::logDebug("[quic] [stream {}] Stream shutdown complete", streamId ? *streamId : 0);
				self->quic_->StreamClose(stream);
				break;
			}

			default: {
				settings::Logger::logDebug("[quic] [stream {}] Unhandled stream event 0x{:x}", streamId ? *streamId : 0,
				                           static_cast<unsigned>(event->Type));
				break;
			}
		}

		return QUIC_STATUS_SUCCESS;
	}

	void QuicCommunication::senderLoop() {
		settings::Logger::logDebug("[quic] Sender thread loop started");

		while (connectionState_.load() == ConnectionState::CONNECTED) {
			std::shared_ptr<ExternalProtocol::ExternalClient> msg;

			std::unique_lock<std::mutex> lock(outboundMutex_);

			settings::Logger::logDebug("[quic] Sender thread loop waiting for outbound queue");
			outboundCv_.wait(lock, [this] {
				return !outboundQueue_.empty() ||
				       connectionState_.load() != ConnectionState::CONNECTED;
			});

			if (connectionState_.load() != ConnectionState::CONNECTED) {
				break;
			}

			settings::Logger::logDebug("[quic] Sender thread loop sending outbound queue");
			msg = outboundQueue_.front();
			outboundQueue_.pop();
			lock.unlock();

			sendViaQuicStream(msg);
		}
	}

	std::optional<uint64_t> QuicCommunication::getStreamId(HQUIC stream) {
		uint64_t streamId = 0;
		uint32_t streamIdLen = sizeof(streamId);

		if (QUIC_FAILED(quic_->GetParam(
			stream,
			QUIC_PARAM_STREAM_ID,
			&streamIdLen,
			&streamId))) {
			return std::nullopt;
		}

		return streamId;
	}

	std::string QuicCommunication::getProtocolSettingsString(
		const structures::ExternalConnectionSettings &settings,
		std::string_view key
	) {
		const auto &raw = settings.protocolSettings.at(std::string(key));

		if (nlohmann::json::accept(raw)) {
			auto j = nlohmann::json::parse(raw);
			if (j.is_string()) {
				return j.get<std::string>();
			}
		}
		return raw;
	}
}
