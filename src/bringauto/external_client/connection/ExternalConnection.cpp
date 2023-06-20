#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>
#include <bringauto/logging/Logger.hpp>


namespace bringauto::external_client::connection {
using log = bringauto::logging::Logger;

ExternalConnection::ExternalConnection( const std::shared_ptr <structures::GlobalContext>& context,
									    const structures::ExternalConnectionSettings& settings,
									    const std::string& company,
									    const std::string& vehicleName,
									    const std::shared_ptr<structures::AtomicQueue<InternalProtocol::DeviceCommand>>& commandQueue){
	commandQueue_ = commandQueue;
    modules_ = settings.modules;
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

	// TODO listening thread
}

void ExternalConnection::sendStatus(const InternalProtocol::DeviceStatus &status, ExternalProtocol::Status::DeviceState deviceState){
    if (not isConnected){
        initializeConnection();
    }
}

void ExternalConnection::initializeConnection(){
    if (communicationChannel_->initializeConnection() == OK) {
		isConnected = true;
	}
    std::vector<device_identification> devices{};
    for(const auto &moduleNumber : modules_){
        struct buffer unique_devices{};
        int ret = context->statusAggregators.find(moduleNumber)->second->get_unique_devices(&unique_devices);
        if (ret <= 0){
            log::logError("Module does not have any devices");
            throw std::runtime_error("");
        }
        // devices.emplace(unique_devices.data);
        connectMessageHandle(devices);
        statusMessageHandle(devices);
        commandMessageHandle(devices);
    }
}

void ExternalConnection::connectMessageHandle(std::vector<device_identification> devices){
    //generate session id
    ExternalProtocol::Connect *connect = new ExternalProtocol::Connect();
    connect->add_devices();
    connect->set_allocated_sessionid(new std::string(sessionId_));
    connect->set_allocated_vehiclename(new std::string(vehicleName_));
    connect->set_allocated_company(new std::string(company_));
    ExternalProtocol::ExternalClient *message = new ExternalProtocol::ExternalClient();
    message->set_allocated_connect(connect);
    communicationChannel_->sendMessage(message);

    const auto connectResponsemsg = communicationChannel_->receiveMessage();
    if (not connectResponsemsg->has_connectresponse()){
        throw std::runtime_error("");
    }
    if (connectResponsemsg->connectresponse().sessionid() != sessionId_){
        throw std::runtime_error("Bad session id in connect response");
    }
    if (connectResponsemsg->connectresponse().type() == ExternalProtocol::ConnectResponse_Type_ALREADY_LOGGED){
        throw std::runtime_error("Already logged in");
    }
}

void ExternalConnection::statusMessageHandle(std::vector<device_identification> devices){
    for (const auto &device : devices){
        struct buffer statusBuffer{};
        const auto &lastStatus = errorAggregators[device.module].get_last_status(&statusBuffer, device);
    }
}

void ExternalConnection::commandMessageHandle(std::vector<device_identification> devices){

}

void ExternalConnection::endConnection(){

}



}
