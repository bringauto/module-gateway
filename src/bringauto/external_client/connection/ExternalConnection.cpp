#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>
#include <bringauto/logging/Logger.hpp>

#include <random>


namespace bringauto::external_client::connection {
using log = bringauto::logging::Logger;

ExternalConnection::ExternalConnection( const std::shared_ptr <structures::GlobalContext>& context,
									    const structures::ExternalConnectionSettings& settings,
									    const std::string& company,
									    const std::string& vehicleName,
									    const std::shared_ptr<structures::AtomicQueue<InternalProtocol::DeviceCommand>>& commandQueue){
	commandQueue_ = commandQueue;
    settings_ = settings;
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
    log::logInfo("Initializing connection to endpoint {}:{} for modules: {}", settings_.serverIp, settings_.port, settings_.modules.data());
    std::vector<device_identification> devices{};
    for(const auto &moduleNumber : settings_.modules){
        struct buffer unique_devices{};
        int ret = context_->statusAggregators.find(moduleNumber)->second->get_unique_devices(&unique_devices);
        if (ret <= 0){
            log::logError("Module does not have any devices");
            throw std::runtime_error("");
        }
        // devices.emplace(unique_devices.data);
        log::logInfo("Connect sequence: 1st step (sending list of devices)");
        connectMessageHandle(devices);
        log::logInfo("Connect sequence: 2nd step (sending statuses of all connected devices)");
        statusMessageHandle(devices);
        log::logInfo("Connect sequence: 3rd step (receiving commands for devices in previous step)");
        commandMessageHandle(devices);
    }
}

void ExternalConnection::connectMessageHandle(std::vector<device_identification> devices){
    setSessionId();

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
        throw std::runtime_error("Message has not type connect response");
    }
    if (connectResponsemsg->connectresponse().sessionid() != sessionId_){
        throw std::runtime_error("Bad session id in connect response");
    }
    if (connectResponsemsg->connectresponse().type() == ExternalProtocol::ConnectResponse_Type_ALREADY_LOGGED){
        throw std::runtime_error("Already logged in");
    }
    log::logInfo("");
}

void ExternalConnection::statusMessageHandle(std::vector<device_identification> devices){
    for (const auto &device : devices){
        struct buffer statusBuffer{};
        const auto &lastStatus = errorAggregators[device.module].get_last_status(&statusBuffer, device);
    }
}

void ExternalConnection::commandMessageHandle(std::vector<device_identification> devices){

}

void ExternalConnection::setSessionId(){
    static auto& chrs = "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    for(int i = 0; i < KEY_LENGHT; i++){
        sessionId_ += chrs[pick(rg)];
    }
    sessionId_ = carId_ + sessionId_;
}

void ExternalConnection::endConnection(){

}



}
