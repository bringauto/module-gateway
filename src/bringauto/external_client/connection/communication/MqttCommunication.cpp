#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>

#include <bringauto/logging/Logger.hpp>



namespace bringauto::external_client::connection::communication {

MqttCommunication::MqttCommunication(structures::ExternalConnectionSettings &settings, std::string company,
									 std::string vehicleName): ICommunicationChannel(
		settings) {
	publishTopic_ = createPublishTopic(company, vehicleName);
	subscribeTopic_ = createSubscribeTopic(company, vehicleName);

	if (settings.protocolSettings.contains("ssl") && settings.protocolSettings["ssl"] == "true") { 	// TODO move settings constants somewhere
		// TODO check true string, or use lowercase
	}
}

// TODO parse settings, create Topic etc.


void MqttCommunication::connect(std::string topic) {
	std::string address = { settings_.serverIp
							+ ":" + std::to_string(settings_.port) };
	client_->connect();
}

MqttCommunication::~MqttCommunication() {
	closeConnection();
}

int MqttCommunication::initializeConnection() {
	try {
		connect(std::string());
	} catch(std::exception &e) {
		logging::Logger::logError("Unable to connect to MQTT {}", e.what());
	}
	return 0;
}

int MqttCommunication::sendMessage() {
	// client_->publish("test", );
	return 0;
}

void MqttCommunication::closeConnection() {
	if(client_ == nullptr) {
		return;
	}
	client_->disconnect();
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