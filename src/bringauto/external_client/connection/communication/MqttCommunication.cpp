#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>

#include <bringauto/logging/Logger.hpp>



namespace bringauto::external_client::connection::communication {

MqttCommunication::MqttCommunication(structures::ExternalConnectionSettings &settings): ICommunicationChannel(
		settings) {}

// TODO address struktura s car, company, address, port
// TODO parse settings, create Topic etc.
void MqttCommunication::init() {

}

void MqttCommunication::connect() {
	client_->connect();
}

MqttCommunication::~MqttCommunication() {
	closeConnection();
}

int MqttCommunication::initializeConnection() {
	try {
		connect();
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

}