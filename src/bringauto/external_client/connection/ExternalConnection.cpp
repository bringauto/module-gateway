#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>
#include <bringauto/common_utils/ProtocolUtils.hpp>

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
		ExternalProtocol::ExternalClient externalMessage;
		ExternalProtocol::Status* status = externalMessage.mutable_status();
        struct buffer errorBuffer{};
        const auto &lastErrorStatusRc = errorAggregators[device.module].get_last_status(&errorBuffer, device);
		if (lastErrorStatusRc == OK) {
			status->set_errormessage(errorBuffer.data, errorBuffer.size_in_bytes);
		} else if (lastErrorStatusRc == DEVICE_NOT_REGISTERED) {
			logging::Logger::logWarning("Device is not registered in error aggregator: {} {}", device.device_role, device.device_name);
		}

		struct buffer statusBuffer {};
		const auto &lastStatusRc = context->statusAggregators[device.module]->get_aggregated_status(&statusBuffer, device);

		auto deviceStatus = status->mutable_devicestatus();
		auto deviceMsg = deviceStatus->mutable_device();
		deviceMsg->CopyFrom(common_utils::ProtocolUtils::CreateDevice(device.module, device.device_type, device.device_role, device.device_name, device.priority));
		if (lastStatusRc == OK) {

			deviceStatus->set_statusdata(statusBuffer.data, statusBuffer.size_in_bytes);
		}
		status->set_devicestate(ExternalProtocol::Status_DeviceState_CONNECTING);
		status->set_sessionid(sessionId_);
		status->set_messagecounter(getNextStatusCounter());
		communicationChannel_->sendMessage(&externalMessage);

		deallocate(&errorBuffer);
		deallocate(&statusBuffer);
    }
}

void ExternalConnection::commandMessageHandle(std::vector<device_identification> devices){

}

void ExternalConnection::endConnection(){

}



}
