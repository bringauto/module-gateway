#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>



namespace bringauto::external_client::connection {

ExternalConnection::ExternalConnection(std::shared_ptr <structures::GlobalContext> &context,
									   const structures::ExternalConnectionSettings &settings,
									   std::string company,
									   std::string vehicleName,
									   const std::shared_ptr<structures::AtomicQueue < InternalProtocol::DeviceCommand>>& commandQueue){ // TODO add company
	commandQueue_ = commandQueue;
	for (const auto& moduleNum : settings.modules) {
		errorAggregators[moduleNum] = ErrorAggregator();
		errorAggregators[moduleNum].init_error_aggregator(context->moduleLibraries[moduleNum]);
	}
	switch (settings.protocolType) {
		case structures::ProtocolType::MQTT:
			communicationChannel_ = std::make_unique<communication::MqttCommunication>(settings, company, vehicleName);
			break;
		case structures::ProtocolType::INVALID:
			break;
	}
	if (communicationChannel_->initializeConnection() == OK) {
		isConnected = true;
	}

	// TODO listening thread
}

}
