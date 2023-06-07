#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>



bringauto::external_client::connection::ExternalConnection::ExternalConnection(
		bringauto::structures::GlobalContext context) {

	//structures::ExternalConnectionSettings settings {};
	//communicationChannel_ = std::make_unique<communication::MqttCommunication>(settings); TODO something like this, but it does not work
}
