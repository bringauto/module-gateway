#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/logging/Logger.hpp>



namespace bringauto::external_client::connection::communication {

MqttCommunication::~MqttCommunication() {
	closeConnection();
}

void MqttCommunication::setProperties(const std::string &company, const std::string &vehicleName) {
	publishTopic_ = createPublishTopic(company, vehicleName);
	subscribeTopic_ = createSubscribeTopic(company, vehicleName);
	clientId_ = createClientId(company, vehicleName);

	serverAddress_ = { settings_.serverIp
					   + ":" + std::to_string(settings_.port) };

	if(settings_.protocolSettings.contains(std::string(settings::Constants::SSL)) && settings_.protocolSettings[std::string(settings::Constants::SSL)] == "true") {
		if(settings_.protocolSettings.contains(std::string(settings::Constants::CA_FILE))
		   && settings_.protocolSettings.contains(std::string(settings::Constants::CLIENT_CERT))
		   && settings_.protocolSettings.contains(std::string(settings::Constants::CLIENT_KEY))
				) {
			serverAddress_ = "ssl://" + serverAddress_;
			auto sslopts = mqtt::ssl_options_builder()
					.trust_store(settings_.protocolSettings[std::string(settings::Constants::CA_FILE)])
					.private_key(settings_.protocolSettings[std::string(settings::Constants::CLIENT_KEY)])
					.key_store(settings_.protocolSettings[std::string(settings::Constants::CLIENT_CERT)])
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
	client_ = std::make_unique<mqtt::async_client>(serverAddress_, clientId_,
												   mqtt::create_options(MQTTVERSION_5)); // TODO sometimes throw SIGSEGV

	client_->start_consuming();
	mqtt::token_ptr conntok = client_->connect(connopts_);
	conntok->wait();
	logging::Logger::logInfo("Connected to MQTT server {}", serverAddress_);

	client_->subscribe(subscribeTopic_, qos)->get_subscribe_response();
}

int MqttCommunication::initializeConnection() {
	if(client_ != nullptr && client_->is_connected()) {
		return 0;
	} else if(client_ != nullptr) {
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
	const auto size = message->ByteSizeLong();
	auto buffer = std::make_unique<uint8_t>(size);

	message->SerializeToArray(buffer.get(), static_cast<int>(size));
	client_->publish(publishTopic_, buffer.get(), size, qos, false);
	return 0;
}

std::shared_ptr <ExternalProtocol::ExternalServer> MqttCommunication::receiveMessage() {
	if(client_ == nullptr) {
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

	auto ptr = std::make_shared<ExternalProtocol::ExternalServer>();
	ptr->ParseFromString(msg->get_payload_str());
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

std::string MqttCommunication::createClientId(const std::string &company, const std::string &vehicleName) {
	return company + std::string("/") + vehicleName; // TODO should have session ID or that is only inside protocol?
}

std::string MqttCommunication::createPublishTopic(const std::string &company, const std::string &vehicleName) {
	return company + std::string("/") + vehicleName + std::string("/module_gateway");
}

std::string MqttCommunication::createSubscribeTopic(const std::string &company, const std::string &vehicleName) {
	return company + std::string("/") + vehicleName + std::string("/external_server");
}


}