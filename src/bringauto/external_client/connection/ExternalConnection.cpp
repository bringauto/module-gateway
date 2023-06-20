#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>
#include <bringauto/common_utils/ProtocolUtils.hpp>
#include <bringauto/utils/utils.hpp>

#include <bringauto/logging/Logger.hpp>

#include <random>



namespace bringauto::external_client::connection {
using log = bringauto::logging::Logger;

ExternalConnection::ExternalConnection(const std::shared_ptr <structures::GlobalContext> &context,
									   const structures::ExternalConnectionSettings &settings,
									   const std::string &company,
									   const std::string &vehicleName,
									   const std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> &commandQueue)
		: context_ { context }, settings_ { settings } {
	commandQueue_ = commandQueue;
	for(const auto &moduleNum: settings.modules) {
		errorAggregators[moduleNum] = ErrorAggregator();
		errorAggregators[moduleNum].init_error_aggregator(context->moduleLibraries[moduleNum]);
	}
	switch(settings.protocolType) {
		case structures::ProtocolType::MQTT:
			communicationChannel_ = std::make_unique<communication::MqttCommunication>(settings, company, vehicleName);
			break;
		case structures::ProtocolType::INVALID:
			break;
	}

	// TODO listening thread
}

void ExternalConnection::sendStatus(const InternalProtocol::DeviceStatus &status,
									ExternalProtocol::Status::DeviceState deviceState) {
	if(not isConnected) {
		initializeConnection();
	}
}

void ExternalConnection::initializeConnection() {
	if(communicationChannel_->initializeConnection() == OK) {
		isConnected = true;
	}
	log::logInfo("Initializing connection to endpoint {}:{}", settings_.serverIp, settings_.port);

	std::vector <device_identification> devices {};
	for(const auto &moduleNumber: settings_.modules) {
		struct buffer unique_devices {};
		int ret = context_->statusAggregators[moduleNumber]->get_unique_devices(&unique_devices);
		if(ret <= 0) {
			log::logWarning("Module {} does not have any devices", moduleNumber);
			continue;
		}
		std::string devicesString { static_cast<char *>(unique_devices.data), unique_devices.size_in_bytes };
		deallocate(&unique_devices);
		auto devicesVec = utils::splitString(devicesString, ',');
		for(const auto &device: devicesVec) {
			devices.push_back(utils::mapToDeviceId(device));
		}
	}

	log::logInfo("Connect sequence: 1st step (sending list of devices)");
	connectMessageHandle(devices);
	log::logInfo("Connect sequence: 2nd step (sending statuses of all connected devices)");
	statusMessageHandle(devices);
	log::logInfo("Connect sequence: 3rd step (receiving commands for devices in previous step)");
	commandMessageHandle(devices);
}

void ExternalConnection::connectMessageHandle(const std::vector <device_identification> &devices) {
	setSessionId();

	ExternalProtocol::Connect *connect = new ExternalProtocol::Connect();
	for(const auto &deviceId: devices) {
		auto connectDevice = connect->add_devices();
		InternalProtocol::Device device {};
		device.set_devicename(deviceId.device_name);
		device.set_devicerole(deviceId.device_role);
		device.set_devicetype(deviceId.device_type);
		device.set_module(InternalProtocol::Device_Module_CAR_ACCESSORY_MODULE);
		connectDevice->CopyFrom(device);
	}
	connect->set_sessionid(sessionId_);
	connect->set_vehiclename(vehicleName_);
	connect->set_company(company_);
	ExternalProtocol::ExternalClient *message = new ExternalProtocol::ExternalClient();
	message->set_allocated_connect(connect);
	communicationChannel_->sendMessage(message);

	const auto connectResponsemsg = communicationChannel_->receiveMessage();
	if(not connectResponsemsg->has_connectresponse()) {
		throw std::runtime_error("Message has not type connect response");
	}
	if(connectResponsemsg->connectresponse().sessionid() != sessionId_) {
		throw std::runtime_error("Bad session id in connect response");
	}
	if(connectResponsemsg->connectresponse().type() == ExternalProtocol::ConnectResponse_Type_ALREADY_LOGGED) {
		throw std::runtime_error("Already logged in");
	}
}

void ExternalConnection::statusMessageHandle(const std::vector <device_identification> &devices) {
	for(const auto &device: devices) {
		struct buffer errorBuffer {};
		const auto &lastErrorStatusRc = errorAggregators[device.module].get_last_status(&errorBuffer, device);
		if(lastErrorStatusRc == DEVICE_NOT_REGISTERED) {
			logging::Logger::logWarning("Device is not registered in error aggregator: {} {}", device.device_role,
										device.device_name);
		}

		struct buffer statusBuffer {};
		const auto &lastStatusRc = context_->statusAggregators[device.module]->get_aggregated_status(&statusBuffer,
																									 device);

		if(lastStatusRc != OK) {
			logging::Logger::logWarning("Cannot obtain status for device: {} {}", device.device_role,
										device.device_name);
		}
		auto externalMessage = common_utils::ProtocolUtils::CreateExternalClientStatus(sessionId_,
																					   ExternalProtocol::Status_DeviceState_CONNECTING,
																					   getNextStatusCounter(),
																					   statusBuffer, device,
																					   errorBuffer);
		communicationChannel_->sendMessage(&externalMessage); // TODO add to acknowledge

		deallocate(&errorBuffer);
		deallocate(&statusBuffer);
	}
}

void ExternalConnection::commandMessageHandle(std::vector <device_identification> devices) {

}

void ExternalConnection::setSessionId() {
	static auto &chrs = "0123456789"
						"abcdefghijklmnopqrstuvwxyz"
						"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	thread_local
	static std::mt19937 rg { std::random_device {}() };
	thread_local
	static std::uniform_int_distribution <std::string::size_type> pick(0, sizeof(chrs) - 2);

	for(int i = 0; i < KEY_LENGHT; i++) {
		sessionId_ += chrs[pick(rg)];
	}
	sessionId_ = carId_ + sessionId_;
}

u_int32_t ExternalConnection::getNextStatusCounter() {
	return ++clientMessageCounter_;
}

void ExternalConnection::endConnection() {

}


}
