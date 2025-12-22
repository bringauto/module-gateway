#include <bringauto/external_client/connection/communication/QuicCommunication.hpp>
#include <bringauto/settings/Constants.hpp>

#include <msquic.h>

#include <fstream>

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

	bool QuicCommunication::sendMessage(ExternalProtocol::ExternalClient *message) {
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

		const QUIC_STATUS status = quic_->StreamSend(
			stream,
			buf,
			1,
			QUIC_SEND_FLAG_START | QUIC_SEND_FLAG_FIN,
			buf
		);

		if (QUIC_FAILED(status)) {
			delete[] buf->Buffer;
			delete buf;

			settings::Logger::logError("[quic] Failed to send QUIC stream; QUIC_STATUS => {:x}", status);
			return false;
		}

		return true;
	}

	std::shared_ptr<ExternalProtocol::ExternalServer> QuicCommunication::receiveMessage() {
		return nullptr;

		// TODO: Implement

		// Budeme potřebovat?
	}

	/// ---------- Connection ----------

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

	/// ---------- Closing client ----------

	void QuicCommunication::closeConnection() {
		if (! connection_) {
			return;
		}

		quic_->ConnectionShutdown(connection_, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);

		// Waiting for QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE then continue in connectionCallback
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

		settings::Logger::logInfo("[quic] Connection stopped");
	}

	/// ---------- Callbacks ----------

	QUIC_STATUS QUIC_API QuicCommunication::connectionCallback(HQUIC connection, void* context, QUIC_CONNECTION_EVENT* event) {
		auto* self = static_cast<QuicCommunication*>(context);

		switch (event->Type) {
			case QUIC_CONNECTION_EVENT_CONNECTED: {
				settings::Logger::logInfo("[quic] Connected to server");

				self->connectionState_ = ConnectionState::CONNECTED;
				// self->tryFlushQueue();
				break;
			}

			case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
				settings::Logger::logInfo("[quic] Connection shutdown complete");

				self->quic_->ConnectionClose(connection);

				self->connection_ = nullptr;
				self->connectionState_ = ConnectionState::DISCONNECTED;
				break;
			}

			case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
				settings::Logger::logWarning("[quic] Connection shutdown initiated");
				self->connectionState_ = ConnectionState::CLOSING;
				break;

			default: {
				settings::Logger::logInfo("[quic] Unexpected connection event 0x{:x}", static_cast<unsigned>(event->Type));
				break;
			}
		}

		return QUIC_STATUS_SUCCESS;
	}

	QUIC_STATUS QUIC_API QuicCommunication::streamCallback(HQUIC stream, void* context, QUIC_STREAM_EVENT* event) {
	    auto* self = static_cast<QuicCommunication*>(context);

	    switch (event->Type) {
	        case QUIC_STREAM_EVENT_RECEIVE: {
	        	settings::Logger::logInfo("[quic] Received {:d} bytes", event->RECEIVE.TotalBufferLength);

	            //quicHandle(event->RECEIVE.Buffers, event->RECEIVE.BufferCount);
	            self->quic_->StreamReceiveComplete(stream, event->RECEIVE.TotalBufferLength);
	            break;
	        }

	        case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN: {
	        	settings::Logger::logInfo("[quic] Peer stream send shutdown");
	            self->quic_->StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_GRACEFUL, 0);
	            break;
	        }

	        case QUIC_STREAM_EVENT_START_COMPLETE: {
	        	settings::Logger::logInfo("[quic] Stream start completed");
	            break;
	        }

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

	        case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE: {
	        	settings::Logger::logInfo("[quic] Stream shutdown complete");
	            self->quic_->StreamClose(stream);
	            break;
	        }

	        default: {
	        	settings::Logger::logInfo("[quic] Unexpected stream event 0x{:x}", static_cast<unsigned>(event->Type));
	            break;
	        }
	    }

	    return QUIC_STATUS_SUCCESS;
	}
}
