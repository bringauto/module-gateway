#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/logging/Logger.hpp>



namespace bringauto::external_client::connection::communication {

MqttCommunication::MqttCommunication(const structures::ExternalConnectionSettings &settings)
	: ICommunicationChannel(settings) {
	
	connopts_.set_mqtt_version(MQTTVERSION_3_1_1);
	connopts_.set_keep_alive_interval(settings::MqttConstants::keepalive);
	connopts_.set_automatic_reconnect(settings::MqttConstants::automatic_reconnect);
	connopts_.set_connect_timeout(std::chrono::milliseconds(settings::MqttConstants::connect_timeout));
	connopts_.set_max_inflight(settings::MqttConstants::max_inflight);

	logging::Logger::logInfo("MQTT communication parameters: keepalive: {}, automatic_reconnect: {}, connect_timeout: {}, "
							 "max_inflight: {}, receive_message_timeout: {}", settings::MqttConstants::keepalive,
							 settings::MqttConstants::automatic_reconnect, settings::MqttConstants::connect_timeout,
							 settings::MqttConstants::max_inflight, settings::receive_message_timeout.count());
}

MqttCommunication::~MqttCommunication() {
	closeConnection();
}

void MqttCommunication::setProperties(const std::string& company, const std::string& vehicleName) {
	publishTopic_ = createPublishTopic(company, vehicleName);
	subscribeTopic_ = createSubscribeTopic(company, vehicleName);
	clientId_ = createClientId(company, vehicleName);

	serverAddress_ = {
		settings_.serverIp
		+ ":" + std::to_string(settings_.port)
	};

	if (settings_.protocolSettings.contains(std::string(settings::Constants::SSL)) &&
	    settings_.protocolSettings[std::string(settings::Constants::SSL)] == "true") {
		if (settings_.protocolSettings.contains(std::string(settings::Constants::CA_FILE))
		    && settings_.protocolSettings.contains(std::string(settings::Constants::CLIENT_CERT))
		    && settings_.protocolSettings.contains(std::string(settings::Constants::CLIENT_KEY))
		) {
			serverAddress_ = "ssl://" + serverAddress_;
			const auto sslopts = mqtt::ssl_options_builder()
					.trust_store(settings_.protocolSettings[std::string(settings::Constants::CA_FILE)])
					.private_key(settings_.protocolSettings[std::string(settings::Constants::CLIENT_KEY)])
					.key_store(settings_.protocolSettings[std::string(settings::Constants::CLIENT_CERT)])
					.error_handler([](const std::string& msg) {
						logging::Logger::logError("MQTT: SSL Error: {}", msg);
					})
					.finalize();
			connopts_.set_ssl(sslopts);
		}
		else {
			logging::Logger::logError(
				"MQTT: Settings doesn't contain all required files for SSL. SSL will not be used in this connection.");
		}
	}
}

void MqttCommunication::initializeConnection() {
	if (client_ != nullptr && client_->is_connected()) {
		return;
	}
	else if (client_ != nullptr) {
		closeConnection();
	}
	connect();
}

void MqttCommunication::connect() {
	client_ = std::make_unique<mqtt::async_client>(serverAddress_, clientId_,
	                                               mqtt::create_options(MQTTVERSION_3_1_1, 20));
	if (client_ == nullptr) {
		throw std::runtime_error {"Mqtt client could not be created"};
	}

	client_->start_consuming();

	const auto conntok = client_->connect(connopts_);
	conntok->wait();
	logging::Logger::logInfo("Connected to MQTT server {}", serverAddress_);

	const auto substok = client_->subscribe(subscribeTopic_, qos);
	substok->wait();
}

bool MqttCommunication::sendMessage(ExternalProtocol::ExternalClient* message) {
	if (client_ == nullptr || not client_->is_connected()) {
		logging::Logger::logError("Mqtt client is not initialized or connected to the server");
		return false;
	}
	const auto size = message->ByteSizeLong();
	const auto buffer = std::make_unique<uint8_t[]>(size);

	message->SerializeToArray(buffer.get(), static_cast<int>(size));
	client_->publish(publishTopic_, buffer.get(), size, qos, false);
	return true;
}

std::shared_ptr<ExternalProtocol::ExternalServer> MqttCommunication::receiveMessage() {
	if(client_ == nullptr) {
		return nullptr;
	}
	mqtt::const_message_ptr msg { nullptr };
	if(client_->is_connected()) {
		msg = client_->try_consume_message_for(settings::receive_message_timeout);
	}

	if(!msg) {
		return nullptr;
	}

	auto ptr = std::make_shared<ExternalProtocol::ExternalServer>();
	ptr->ParseFromString(msg->get_payload_str());
	return ptr;
}

void MqttCommunication::closeConnection() {
	if(not client_) {
		return;
	}
	if(client_->is_connected()) {
		client_->unsubscribe(subscribeTopic_);
		client_->disconnect();
	}
	client_.reset();
}

std::string MqttCommunication::createClientId(const std::string &company, const std::string &vehicleName) {
	return company + std::string("/") + vehicleName;
}

std::string MqttCommunication::createPublishTopic(const std::string &company, const std::string &vehicleName) {
	return createClientId(company, vehicleName) + std::string("/module_gateway");
}

std::string MqttCommunication::createSubscribeTopic(const std::string &company, const std::string &vehicleName) {
	return createClientId(company, vehicleName) + std::string("/external_server");
}


}