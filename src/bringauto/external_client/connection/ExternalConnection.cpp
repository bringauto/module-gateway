#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>



namespace bringauto::external_client::connection {

ExternalConnection::ExternalConnection(const structures::ExternalConnectionSettings &settings) { // TODO add company

	switch (settings.protocolType) {
		case structures::ProtocolType::MQTT:
			communicationChannel_ = std::make_unique<communication::MqttCommunication>(settings, "", "");
			break;
		case structures::ProtocolType::INVALID:
			break;
	}
}


void ExternalConnection::send(){

}

}
