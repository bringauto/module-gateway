#include <bringauto/external_client/connection/communication/QuicCommunication.hpp>
#include <bringauto/settings/Constants.hpp>

#include <msquic.h>

#include <fstream>
#include <thread>

#include "bringauto/settings/LoggerId.hpp"


namespace bringauto::external_client::connection::communication {

	QuicCommunication::QuicCommunication(const structures::ExternalConnectionSettings &settings, const std::string &company,
		const std::string &vehicleName) : ICommunicationChannel(settings),
		certFile_(settings.protocolSettings.at(std::string(settings::Constants::CLIENT_CERT))),
		keyFile_(settings.protocolSettings.at(std::string(settings::Constants::CLIENT_KEY))),
		caFile_(settings.protocolSettings.at(std::string(settings::Constants::CA_FILE)))
	{
		alpn_ = "sample";
		alpnBuffer_.Buffer = reinterpret_cast<uint8_t*>(alpn_.data());
		alpnBuffer_.Length = static_cast<uint32_t>(alpn_.size());

		settings::Logger::logInfo("[quic] Initialize QUIC connection to {}:{} for {}/{}", settings.serverIp, settings.port, company, vehicleName);

		loadMsQuic();
		initRegistration("module-gateway-quic-client");
		initConfiguration();
	}

	QuicCommunication::~QuicCommunication() {
		stop();
	}

	void QuicCommunication::initializeConnection() {
		settings::Logger::logInfo("[quic] Connecting to server when {}", toString(connectionState_));


		ConnectionState expected = ConnectionState::DISCONNECTED;
		if (! connectionState_.compare_exchange_strong(expected, ConnectionState::CONNECTING, std::memory_order_acq_rel)) {
			return;
		}

		QUIC_STATUS status = quic_->ConnectionOpen(registration_, connectionCallback, this, &connection_);
		if (QUIC_FAILED(status)) {
			settings::Logger::logError("[quic] Failed to open QUIC connection; QUIC_STATUS => {:x}", status);
		}

		status = quic_->ConnectionStart(
			connection_,
			config_,
			QUIC_ADDRESS_FAMILY_INET,
			settings_.serverIp.c_str(),
			settings_.port
		);

		if (QUIC_FAILED(status)) {
			settings::Logger::logError("[quic] Failed to start QUIC connection; QUIC_STATUS => {:x}", status);
		}

		connectionState_ = ConnectionState::CONNECTING;
	}

	bool QuicCommunication::sendMessage(ExternalProtocol::ExternalClient* message) {
		settings::Logger::logInfo("[quic] Enqueueing message");
		auto copy = std::make_shared<ExternalProtocol::ExternalClient>(*message);

		{
			std::lock_guard<std::mutex> lock(outboundMutex_);
			outboundQueue_.push(std::move(copy));
		}
		settings::Logger::logInfo("[quic] Notifying sender thread");
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
		}
	}

	void QuicCommunication::initRegistration(const char* appName) {
		QUIC_REGISTRATION_CONFIG config {};
		config.AppName = const_cast<char*>(appName);
		config.ExecutionProfile = QUIC_EXECUTION_PROFILE_LOW_LATENCY;

		QUIC_STATUS status = quic_->RegistrationOpen(&config, &registration_);
		if (QUIC_FAILED(status)) {
			settings::Logger::logError("[quic] Failed to open QUIC registration; QUIC_STATUS => {:x}", status);
		}
	}

	void QuicCommunication::initConfiguration() {
		configurationOpen(nullptr);

		QUIC_CERTIFICATE_FILE certificate {};
		certificate.CertificateFile = certFile_.c_str();
		certificate.PrivateKeyFile = keyFile_.c_str();

		QUIC_CREDENTIAL_CONFIG credential {};
		credential.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
		credential.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_SET_CA_CERTIFICATE_FILE;
		credential.CertificateFile = &certificate;
		credential.CaCertificateFile = caFile_.c_str();

		configurationLoadCredential(&credential);
	}

	void QuicCommunication::configurationOpen(const QUIC_SETTINGS* settings) {
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
		}
	}

	void QuicCommunication::configurationLoadCredential(const QUIC_CREDENTIAL_CONFIG* credential) const {
		const QUIC_STATUS status = quic_->ConfigurationLoadCredential(config_, credential);
		if (QUIC_FAILED(status)) {
			settings::Logger::logError("[quic] Failed to load QUIC credential; QUIC_STATUS => {:x}", status);
		}
	}

	void QuicCommunication::closeConnection() {
		if (! connection_) {
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
			settings::Logger::logInfo("[quic] Moving message to inboundQueue");
			inboundQueue_.push(std::move(msg));
		}
		settings::Logger::logInfo("[quic] Notifying receiver thread");
		inboundCv_.notify_one();
	}

	bool QuicCommunication::sendViaQuicStream(const std::shared_ptr<ExternalProtocol::ExternalClient>& message) {
		settings::Logger::logInfo("[quic] Sending message");

		HQUIC stream { nullptr };
		if (QUIC_FAILED(quic_->StreamOpen(connection_, QUIC_STREAM_OPEN_FLAG_NONE, streamCallback, this, &stream))) {
			settings::Logger::logError("[quic] StreamOpen failed");
			return false;
		}

		const auto size = message->ByteSizeLong();
		const auto buffer = std::make_unique<uint8_t[]>(size);

		if (!message->SerializeToArray(buffer.get(), static_cast<int>(size))) {
			settings::Logger::logError("[quic] Message serialization failed");
			return false;
		}

		auto* buf = new QUIC_BUFFER{};
		buf->Length = static_cast<uint32_t>(size);
		buf->Buffer = new uint8_t[buf->Length];

		std::memcpy(buf->Buffer, buffer.get(), buf->Length);

		const QUIC_STATUS status = quic_->StreamSend(stream, buf, 1,
			/**
			 * START => Simulates quic_->StreamStart before send
			 * FIN => Simulates quic_->StreamShutdown after send
			 */
			QUIC_SEND_FLAG_START | QUIC_SEND_FLAG_FIN,
			buf
		);

		if (QUIC_FAILED(status)) {
			delete[] buf->Buffer;
			delete buf;

			settings::Logger::logError("[quic] Failed to send QUIC stream; QUIC_STATUS => {:x}", status);
			quic_->StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
			return false;
		}

		return true;
	}

	QUIC_STATUS QUIC_API QuicCommunication::connectionCallback(HQUIC connection, void* context, QUIC_CONNECTION_EVENT* event) {
		auto* self = static_cast<QuicCommunication*>(context);

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

				self->connectionState_ = ConnectionState::DISCONNECTED;
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
				settings::Logger::logWarning("[quic] Connection shutdown initiated");
				self->connectionState_ = ConnectionState::CLOSING;
				break;
			}

			default: {
				settings::Logger::logInfo("[quic] Unhandled connection event 0x{:x}", static_cast<unsigned>(event->Type));
				break;
			}
		}

		return QUIC_STATUS_SUCCESS;
	}

	QUIC_STATUS QUIC_API QuicCommunication::streamCallback(HQUIC stream, void* context, QUIC_STREAM_EVENT* event) {
	    auto* self = static_cast<QuicCommunication*>(context);
		auto streamId = self->getStreamId(stream);

	    switch (event->Type) {
	    	/// Raised when the peer sends stream data and MsQuic delivers received bytes to the application.
	    	case QUIC_STREAM_EVENT_RECEIVE: {
	    		settings::Logger::logInfo(
					"[quic] Received {:d} bytes in {:d} buffers",
					event->RECEIVE.TotalBufferLength,
					event->RECEIVE.BufferCount
				);

	    		settings::Logger::logInfo(
					"[quic] [stream {}] Event RECEIVE",
					streamId ? *streamId : 0
				);

	    		std::vector<uint8_t> data;
	    		data.reserve(event->RECEIVE.TotalBufferLength);

	    		for (uint32_t i = 0; i < event->RECEIVE.BufferCount; ++i) {
	    			const auto& b = event->RECEIVE.Buffers[i];
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
	        	settings::Logger::logInfo("[quic] Peer stream send shutdown");
	            break;
	        }

	    	/// Raised when the local send direction is fully shut down
	    	/// and the peer has acknowledged the FIN.
	    	case QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE: {
	    		settings::Logger::logInfo("[quic] [stream {}] Stream send shutdown complete", streamId ? *streamId : 0);
	    		break;
	    	}

	    	/// Raised after StreamStart completes successfully
	    	/// and the stream becomes active with a valid stream ID.
	        case QUIC_STREAM_EVENT_START_COMPLETE: {
	        	settings::Logger::logInfo("[quic] Stream start completed");
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
	            	settings::Logger::logError("[quic] Stream send canceled");
	            } else {
	            	settings::Logger::logInfo("[quic] Stream send completed");
	            }

	            const auto* buf =
	                static_cast<QUIC_BUFFER*>(event->SEND_COMPLETE.ClientContext);

	            if (buf) {
	                delete[] buf->Buffer;
	                delete buf;
	            }

	            break;
	        }

	    	/// Raised when both send and receive directions are closed
	    	/// and the stream lifecycle is fully complete.
	        case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE: {
	        	settings::Logger::logInfo("[quic] Stream shutdown complete");
	            self->quic_->StreamClose(stream);
	            break;
	        }

	        default: {
	        	settings::Logger::logInfo("[quic] Unhandled stream event 0x{:x}", static_cast<unsigned>(event->Type));
	            break;
	        }
	    }

	    return QUIC_STATUS_SUCCESS;
	}

	void QuicCommunication::senderLoop() {
		settings::Logger::logInfo("[quic] Sender thread loop started");
		while (connectionState_.load() == ConnectionState::CONNECTED) {
			std::unique_lock<std::mutex> lock(outboundMutex_);

			settings::Logger::logInfo("[quic] Sender thread loop waiting for outbound queue");
			outboundCv_.wait(lock, [this] {
				return !outboundQueue_.empty() ||
					   connectionState_.load() != ConnectionState::CONNECTED;
			});

			if (connectionState_.load() != ConnectionState::CONNECTED) {
				break;
			}

			settings::Logger::logInfo("[quic] Sender thread loop sending outbound queue");
			auto msg = outboundQueue_.front();
			outboundQueue_.pop();
			lock.unlock();

			sendViaQuicStream(msg);
		}
	}
}
