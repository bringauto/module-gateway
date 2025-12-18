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

		settings::Logger::logInfo("Initialize QUIC connection to {}:{} for {}/{}", settings.serverIp, settings.port, company, vehicleName);

		loadMsQuic();
		initRegistration("module-gateway-quic-client");
		initConfiguration();
	}

	QuicCommunication::~QuicCommunication() {
		stop();
	}

	void QuicCommunication::initializeConnection() {
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

	void QuicCommunication::stop() {
		// if (!isRunning_.exchange(false)) {
		// 	return;
		// }

		if (connection_) {
			quic_->ConnectionShutdown(connection_, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
			quic_->ConnectionClose(connection_);
		}
		if (config_) {
			quic_->ConfigurationClose(config_);
		}
		if (registration_) {
			quic_->RegistrationClose(registration_);
		}
		if (quic_) {
			MsQuicClose(quic_);
		}

		settings::Logger::logInfo("[quic] Connection stopped");

		connection_ = nullptr;
		config_ = nullptr;
		registration_ = nullptr;
		quic_ = nullptr;
	}

	QUIC_STATUS QUIC_API QuicCommunication::connectionCallback(HQUIC connection,
													void* context,
													QUIC_CONNECTION_EVENT* event) {
		auto* self = static_cast<QuicCommunication*>(context);

		switch (event->Type) {
			case QUIC_CONNECTION_EVENT_CONNECTED: {
				settings::Logger::logInfo("[quic] Connected to server");

				// self->connectionState_ = ConnectionState::Connected;
				// self->tryFlushQueue();
				break;
			}

			case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
				settings::Logger::logInfo("[quic] Connection shutdown complete");

				self->quic_->ConnectionClose(connection);
				self->connection_ = nullptr;
				// self->connectionState_ = ConnectionState::Disconnected;
				break;
			}

			default: {
				settings::Logger::logInfo("[quic] Unexpected connection event 0x{:x}", static_cast<unsigned>(event->Type));
				break;
			}
		}

		return QUIC_STATUS_SUCCESS;
	}
}
