#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>
#include <bringauto/settings/Constants.hpp>

#include <bringauto/logging/Logger.hpp>



namespace bringauto::external_client::connection::communication {

MqttCommunication::MqttCommunication(const structures::ExternalConnectionSettings &settings, const std::string& company,
									 const std::string& vehicleName): ICommunicationChannel(
		settings, company, vehicleName) {
	settings_ = settings;
	publishTopic_ = createPublishTopic(company, vehicleName);
	subscribeTopic_ = createSubscribeTopic(company, vehicleName);
	clientId_ = createClientId(company, vehicleName);

	serverAddress_ = { settings_.serverIp
							+ ":" + std::to_string(settings_.port) };

	if (settings_.protocolSettings.contains(settings::SSL) && settings_.protocolSettings[settings::SSL] == "true") {
		if (settings.protocolSettings.contains(settings::CA_FILE)
			&& settings.protocolSettings.contains(settings::CLIENT_CERT)
			&& settings.protocolSettings.contains(settings::CLIENT_KEY)
			) {
			serverAddress_ = "ssl://" + serverAddress_;
			auto sslopts = mqtt::ssl_options_builder()
					.trust_store(settings_.protocolSettings[settings::CA_FILE])
					.private_key(settings_.protocolSettings[settings::CLIENT_KEY])
					.key_store(settings_.protocolSettings[settings::CLIENT_CERT])
					.error_handler([](const std::string &msg) {
						logging::Logger::logError("MQTT: SSL Error: {}", msg);
					})
					.finalize();
			connopts_.set_ssl(sslopts);

		} else {
			logging::Logger::logError("MQTT: Settings doesn't contain all required files for SSL");
			// TODO throw error?? Or maybe move this to connect
		}
	}
}



void MqttCommunication::connect() {
	client_ = std::make_unique<mqtt::async_client>(serverAddress_, clientId_, mqtt::create_options(MQTTVERSION_5)); // TODO do I have to create new client every time?

	client_->start_consuming();
	mqtt::token_ptr conntok = client_->connect(connopts_);
	conntok->wait();
	logging::Logger::logInfo("Connected to MQTT server {}", serverAddress_);

	client_->subscribe(subscribeTopic_, qos)->get_subscribe_response();
}

MqttCommunication::~MqttCommunication() {
	closeConnection();
}

int MqttCommunication::initializeConnection() {
	if (client_ != nullptr && client_->is_connected()) {
		return 0;
	} else if (client_ != nullptr) {
		closeConnection();
	}
	try {
		connect();
	} catch(std::exception &e) {
		logging::Logger::logError("Unable to connect to MQTT: {}", e.what());
		return -1;
	}
	return 0;
}

int MqttCommunication::sendMessage(ExternalProtocol::ExternalClient *message) {
	if(!client_->is_connected() || client_ == nullptr) {
		return -1;
	}
	unsigned int size = message->ByteSizeLong();
	uint8_t buffer[size];
	memset(buffer, '\0', size);
	message->SerializeToArray(buffer, static_cast<int>(size));
	client_->publish(publishTopic_, buffer, size, qos, false);
	return 0;
}

std::shared_ptr<ExternalProtocol::ExternalServer> MqttCommunication::receiveMessage() {
	if (client_ == nullptr) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		return nullptr;
	}
	mqtt::const_message_ptr msg { nullptr };
	if(client_->is_connected()) {
		msg = client_->try_consume_message_for(std::chrono::seconds(10));
	} else {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	if(!msg) {
		return nullptr;
	}

	std::shared_ptr<ExternalProtocol::ExternalServer> ptr { nullptr };

	auto *incomingMessage = new ExternalProtocol::ExternalServer();
	incomingMessage->ParseFromString(msg->get_payload());
	ptr.reset(incomingMessage);
	return ptr;
}

void MqttCommunication::closeConnection() {
	if(client_ == nullptr) {
		return;
	}
	if(client_->is_connected()) {
		client_->unsubscribe(subscribeTopic_);
		client_->disconnect();
	}
	client_.reset();
}

std::string MqttCommunication::createClientId(const std::string& company, const std::string& vehicleName) {
	return company + std::string("/") + vehicleName; // TODO should have session ID or that is only inside protocol?
}

std::string MqttCommunication::createPublishTopic(const std::string &company, const std::string &vehicleName) {
	return company + std::string("/") + vehicleName + std::string("/module_gateway");
}

std::string MqttCommunication::createSubscribeTopic(const std::string &company, const std::string &vehicleName) {
	return company + std::string("/") + vehicleName + std::string("/external_server");
}


}