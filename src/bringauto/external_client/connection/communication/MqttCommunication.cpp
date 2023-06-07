#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>

#include <bringauto/logging/Logger.hpp>



namespace bringauto::external_client::connection::communication {

// TODO address struktura s car, company, address, port
MqttCommunication::MqttCommunication(structures::ExternalConnectionSettings settings) : ICommunicationChannel(settings) {
	// TODO parse settings, create Topic etc.
}

void MqttCommunication::connect() {

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
}

int MqttCommunication::sendMessage() {
	return 0;
}

void MqttCommunication::closeConnection() {
	client_->disconnect();
}

}