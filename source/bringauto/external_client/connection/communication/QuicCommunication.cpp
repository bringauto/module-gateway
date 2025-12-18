#include <bringauto/external_client/connection/communication/QuicCommunication.hpp>
#include <bringauto/settings/Constants.hpp>

#include <msquic.h>

#include <fstream>


namespace bringauto::external_client::connection::communication {

	QuicCommunication::QuicCommunication(const structures::ExternalConnectionSettings &settings, const std::string &company,
		const std::string &vehicleName) : ICommunicationChannel(settings),
		certFile_(settings.protocolSettings.at(std::string(settings::Constants::CLIENT_CERT))),
		keyFile_(settings.protocolSettings.at(std::string(settings::Constants::CLIENT_KEY))),
		caFile_(settings.protocolSettings.at(std::string(settings::Constants::CA_FILE)))
	{
		std::cout << "QuicCommunication for " << vehicleName << "/" << company << std::endl;

		// TODO: Implement

		alpn_ = "sample";
		alpnBuffer_.Buffer = reinterpret_cast<uint8_t*>(alpn_.data());
		alpnBuffer_.Length = static_cast<uint32_t>(alpn_.size());

		QUIC_STATUS status = MsQuicOpen2(&quic_);
		if (QUIC_FAILED(status)) {
			throw std::runtime_error("Failed to open QUIC");
		}

		QUIC_REGISTRATION_CONFIG config {};
		config.AppName = const_cast<char*>("module-gateway-quic-client");
		config.ExecutionProfile = QUIC_EXECUTION_PROFILE_LOW_LATENCY;

		status = quic_->RegistrationOpen(&config, &registration_);
		if (QUIC_FAILED(status)) {
			throw std::runtime_error("Failed to open QUIC registration");
		}

		status = quic_->ConfigurationOpen(
			registration_,
		   &alpnBuffer_,
		   1,
		   nullptr,
		   0,
		   nullptr,
		   &config_
		);

		if (QUIC_FAILED(status)) {
			throw std::runtime_error("Failed to open QUIC configuration");
		}

		certificate_.CertificateFile = certFile_.c_str();
		certificate_.PrivateKeyFile = keyFile_.c_str();

		credential_.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
		credential_.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_SET_CA_CERTIFICATE_FILE;
		credential_.CertificateFile = &certificate_;
		credential_.CaCertificateFile = caFile_.c_str();

		status = quic_->ConfigurationLoadCredential(config_, &credential_);
		if (QUIC_FAILED(status)) {
			throw std::runtime_error("Failed to load QUIC credential");
		}

		status = quic_->ConnectionOpen(registration_, connectionCallback, this, &connection_);
		if (QUIC_FAILED(status)) {
			throw std::runtime_error("Failed to open QUIC connection");
		}

		status = quic_->ConnectionStart(
			connection_,
			config_,
			QUIC_ADDRESS_FAMILY_INET,
			settings.serverIp.c_str(),
			settings.port
		);

		if (QUIC_FAILED(status)) {
			throw std::runtime_error("Failed to start QUIC connection");
		}

		// loadMsQuic();
		// initRegistration("quic_client");
		// initConfiguration();
	}

	QuicCommunication::~QuicCommunication() {
		// TODO: Implement

		// stop()
	}

	void QuicCommunication::initializeConnection() {
		// TODO: Implement

		// initConnection();
		// isRunning_ = true;

	}

	bool QuicCommunication::sendMessage(ExternalProtocol::ExternalClient *message) {
		(void) message;
		return true;

		// TODO: Implement

		// send()
	}

	std::shared_ptr<ExternalProtocol::ExternalServer> QuicCommunication::receiveMessage() {
		return nullptr;

		// TODO: Implement

		// Budeme potřebovat?
	}

	void QuicCommunication::closeConnection() {
		// TODO: Implement

		// stop()
	}

	QUIC_STATUS QUIC_API QuicCommunication::connectionCallback(HQUIC connection,
													void* context,
													QUIC_CONNECTION_EVENT* event) {
		auto* self = static_cast<QuicCommunication*>(context);

		switch (event->Type) {
			case QUIC_CONNECTION_EVENT_CONNECTED: {

				std::cout << "[quic] Connected\n";

				// self->connectionState_ = ConnectionState::Connected;
				// self->tryFlushQueue();
				break;
			}

			case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
				std::cout << "[quic] Connection shutdown complete\n";

				self->quic_->ConnectionClose(connection);
				self->connection_ = nullptr;
				// self->connectionState_ = ConnectionState::Disconnected;
				break;
			}

			// case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED: {
			// 	if (self->zeroRttMode_ == ZeroRttMode::Enabled) {
			// 		self->resumptionTicket_.assign(
			// 			event->RESUMPTION_TICKET_RECEIVED.ResumptionTicket,
			// 			event->RESUMPTION_TICKET_RECEIVED.ResumptionTicket +
			// 				event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength
			// 		);
			//
			// 		std::cout << "[quic] Resumption ticket received!\n";
			// 	}
			// 	break;
			// }

			default: {
				std::cout << "[quic] Connection event type: 0x" << std::hex << event->Type << std::dec << "\n";
				break;
			}
		}

		return QUIC_STATUS_SUCCESS;
	}
}
