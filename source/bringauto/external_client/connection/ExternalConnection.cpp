#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>
#include <bringauto/settings/LoggerId.hpp>

#include <fleet_protocol/module_gateway/error_codes.h>

#include <random>



namespace bringauto::external_client::connection {
using log = settings::Logger;

ExternalConnection::ExternalConnection(const std::shared_ptr<structures::GlobalContext> &context,
									   structures::ModuleLibrary &moduleLibrary,
									   const structures::ExternalConnectionSettings &settings,
									   const std::shared_ptr<structures::AtomicQueue<InternalProtocol::DeviceCommand>> &commandQueue,
									   const std::shared_ptr<structures::AtomicQueue<
											   structures::ReconnectQueueItem>> &reconnectQueue):
		context_ { context },
		moduleLibrary_ { moduleLibrary },
		settings_ { settings },
		commandQueue_ { commandQueue },
		reconnectQueue_ { reconnectQueue } {
	sentMessagesHandler_ = std::make_unique<messages::SentMessagesHandler>(context, [this]() {
		reconnectQueue_->push(structures::ReconnectQueueItem(std::ref(*this), true));
	});
}

void ExternalConnection::init(const std::shared_ptr<communication::ICommunicationChannel> &communicationChannel) {
	for(const auto &moduleNum: settings_.modules) {
		errorAggregators_[moduleNum] = ErrorAggregator();
		errorAggregators_[moduleNum].init_error_aggregator(moduleLibrary_.moduleLibraryHandlers[moduleNum]);
	}
	communicationChannel_ = communicationChannel;
}

void ExternalConnection::sendStatus(const InternalProtocol::DeviceStatus &status,
									ExternalProtocol::Status::DeviceState deviceState,
									const modules::Buffer& errorMessage) {
	const auto &device = status.device();
	const auto &deviceModule = device.module();

	const auto it = errorAggregators_.find(deviceModule);
	if(it == errorAggregators_.end()) {
		log::logError(
				"Status with module number ({}) was passed to external connection, that doesn't support this module",
				static_cast<int>(device.module()));
		return;
	}
	auto &errorAggregator = it->second;
	const auto deviceId = structures::DeviceIdentification(device);
	const auto moduleLibraryHandler = moduleLibrary_.moduleLibraryHandlers.at(deviceModule);

	modules::Buffer lastStatus {};
	const auto isRegistered = errorAggregator.get_last_status(lastStatus, deviceId);
	if (isRegistered == DEVICE_NOT_REGISTERED){
		deviceState = ExternalProtocol::Status_DeviceState_CONNECTING;

		const auto &statusData = status.statusdata();
		const auto statusBuffer = moduleLibraryHandler->constructBuffer(statusData.size());
		if (!statusBuffer.isEmpty()) {
			common_utils::ProtobufUtils::copyStatusToBuffer(status, statusBuffer);
		}
		errorAggregator.add_status_to_error_aggregator(statusBuffer, deviceId);
	}

	switch(deviceState) {
		case ExternalProtocol::Status_DeviceState_CONNECTING:
			sentMessagesHandler_->addDeviceAsConnected(deviceId);
			break;
		case ExternalProtocol::Status_DeviceState_RUNNING:
			if(not sentMessagesHandler_->isDeviceConnected(deviceId)) {
				deviceState = ExternalProtocol::Status_DeviceState_CONNECTING;
				sentMessagesHandler_->addDeviceAsConnected(deviceId);
			}
			break;
		case ExternalProtocol::Status_DeviceState_DISCONNECT:
			sentMessagesHandler_->deleteConnectedDevice(deviceId);
			break;
		case ExternalProtocol::Status_DeviceState_ERROR:
		default:
			log::logError("Status with unsupported deviceState was passed to sendStatus()");
			break;
	}

	auto externalMessage = common_utils::ProtobufUtils::createExternalClientStatus(sessionId_,
																				   deviceState,
																				   getNextStatusCounter(),
																				   status,
																				   errorMessage);
	sentMessagesHandler_->addNotAckedStatus(externalMessage.status());

	if(not communicationChannel_->sendMessage(&externalMessage)){
		deinitializeConnection(false);
	}

	if (errorMessage.isAllocated()) {
		log::logDebug("Sending status with messageCounter '{}' with an aggregated errorMessage", clientMessageCounter_);
	} else {
		log::logDebug("Sending status with messageCounter '{}'", clientMessageCounter_);
	}
}

int ExternalConnection::initializeConnection(const std::vector<structures::DeviceIdentification>& connectedDevices) {
	if(state_.load() == ConnectionState::NOT_INITIALIZED) {
		state_.exchange(ConnectionState::NOT_CONNECTED);
	}

	try {
		communicationChannel_->initializeConnection();
	} catch(std::exception &e) {
		log::logError("Unable to create connection to {}:{} reason: {}", settings_.serverIp, settings_.port, e.what());
		return NOT_OK;
	}
	log::logInfo("Initializing connection to endpoint {}:{}", settings_.serverIp, settings_.port);

	state_.exchange(ConnectionState::CONNECTING);
	log::logInfo("Connect sequence: 1st step (sending list of devices)");
	if(connectMessageHandle(connectedDevices) != OK) {
		log::logError("Connect sequence to server {}:{}, failed in 1st step", settings_.serverIp, settings_.port);
		state_.exchange(ConnectionState::NOT_CONNECTED);
		return NOT_OK;
	}
	log::logInfo("Connect sequence: 2nd step (sending statuses of all connected devices)");
	if(statusMessageHandle(connectedDevices) != OK) {
		log::logError("Connect sequence to server {}:{}, failed in 2nd step", settings_.serverIp, settings_.port);
		state_.exchange(ConnectionState::NOT_CONNECTED);
		return NOT_OK;
	}
	log::logInfo("Connect sequence: 3rd step (receiving commands for devices in previous step)");
	if(commandMessageHandle(connectedDevices) != OK) {
		log::logError("Connect sequence to server {}:{}, failed in 3rd step", settings_.serverIp, settings_.port);
		state_.exchange(ConnectionState::NOT_CONNECTED);
		return NOT_OK;
	}
	listeningThread = std::jthread(&ExternalConnection::receivingHandlerLoop, this);
	state_.exchange(ConnectionState::CONNECTED);
	for(auto &[moduleNum, errorAggregator]: errorAggregators_) {
		errorAggregator.clear_error_aggregator();
	}
	log::logInfo("Connect sequence successful. Server {}:{}", settings_.serverIp, settings_.port);
	return OK;
}

int ExternalConnection::connectMessageHandle(const std::vector<structures::DeviceIdentification> &devices) {
	generateSessionId();

	auto connectMessage = common_utils::ProtobufUtils::createExternalClientConnect(sessionId_, company_, vehicleName_,
																				   devices);
	if(not communicationChannel_->sendMessage(&connectMessage)){
		log::logError("Communication client couldn't send any message");
		return NOT_OK;
	}

	const auto connectResponseMsg = communicationChannel_->receiveMessage();
	if(connectResponseMsg == nullptr) {
		log::logError("Communication client couldn't receive any message");
		return NO_MESSAGE_AVAILABLE;
	}
	if(not connectResponseMsg->has_connectresponse()) {
		log::logError("Received message doesn't have connect response type");
		return NOT_OK;
	}
	if(connectResponseMsg->connectresponse().sessionid() != sessionId_) {
		log::logError("Bad session id in connect response");
		return NOT_OK;
	}
	if(connectResponseMsg->connectresponse().type() == ExternalProtocol::ConnectResponse_Type_ALREADY_LOGGED) {
		log::logError("Already logged in");
		return NOT_OK;
	}
	return OK;
}

int ExternalConnection::statusMessageHandle(const std::vector<structures::DeviceIdentification> &devices) {
	for(const auto &deviceIdentification: devices) {
		const int &deviceModule = deviceIdentification.getModule();
		modules::Buffer errorBuffer {};
		modules::Buffer statusBuffer {};

		const auto &lastErrorStatusRc = errorAggregators_[deviceModule].get_error(errorBuffer, deviceIdentification);
		if(lastErrorStatusRc == DEVICE_NOT_REGISTERED) {
			log::logError("Device is not registered in error aggregator: {} {}",
				deviceIdentification.getDeviceRole(),
				deviceIdentification.getDeviceName());
			return DEVICE_NOT_REGISTERED;
		}

		const int lastStatusRc = errorAggregators_[deviceModule].get_last_status(statusBuffer, deviceIdentification);
		if(lastStatusRc != OK) {
			log::logError("Cannot obtain status for device: {} {}",
				deviceIdentification.getDeviceRole(),
				deviceIdentification.getDeviceName());
			return NO_MESSAGE_AVAILABLE;
		}
		auto deviceStatus = common_utils::ProtobufUtils::createDeviceStatus(deviceIdentification, statusBuffer);
		sendStatus(deviceStatus, ExternalProtocol::Status_DeviceState_CONNECTING, errorBuffer);
	}

	const auto size = devices.size();
	for(unsigned int i = 0; i < size; ++i) {
		const auto statusResponseMsg = communicationChannel_->receiveMessage();
		if(statusResponseMsg == nullptr) {
			log::logError("Communication client couldn't receive any message");
			return NO_MESSAGE_AVAILABLE;
		}
		if(not statusResponseMsg->has_statusresponse()) {
			log::logError("Received message doesn't have status response type");
			return STATUS_INVALID;
		}
		if(statusResponseMsg->statusresponse().type() != ExternalProtocol::StatusResponse_Type_OK) {
			log::logError("Status response does not contain OK");
			return STATUS_INVALID;
		}
		if(statusResponseMsg->statusresponse().sessionid() != sessionId_) {
			log::logError("Bad session id in status response");
			return STATUS_INVALID;
		}
		sentMessagesHandler_->acknowledgeStatus(statusResponseMsg->statusresponse());
	}
	return OK;
}

int ExternalConnection::commandMessageHandle(const std::vector<structures::DeviceIdentification> &devices) {
	const auto size = devices.size();
	for(unsigned int i = 0; i < size; ++i) {
		const auto commandMsg = communicationChannel_->receiveMessage();
		if(commandMsg == nullptr) {
			log::logError("Communication client couldn't receive any message");
			return NO_MESSAGE_AVAILABLE;
		}
		if(not commandMsg->has_command()) {
			log::logError("Received message doesn't have command type");
			return COMMAND_INVALID;
		}
		if(handleCommand(commandMsg->command()) != OK) {
			return NOT_OK;
		}
	}
	return OK;
}

void ExternalConnection::generateSessionId() {
	static const std::string chrs = "0123456789"
									"abcdefghijklmnopqrstuvwxyz"
									"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	thread_local static std::mt19937 rg { std::random_device {}() };
	thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, chrs.size() - 1);

	sessionId_ = "";
	sessionId_.reserve(KEY_LENGTH);
	for(int i = 0; i < KEY_LENGTH; i++) {
		sessionId_ += chrs[pick(rg)];
	}
}

u_int32_t ExternalConnection::getNextStatusCounter() {
	return ++clientMessageCounter_;
}

void ExternalConnection::deinitializeConnection(bool completeDisconnect = false) {
	state_.exchange(ConnectionState::NOT_INITIALIZED);
	clientMessageCounter_ = 0;
	serverMessageCounter_ = 0;
	sentMessagesHandler_->clearAllTimers();

	stopReceiving.exchange(true);
	communicationChannel_->closeConnection();
	if(listeningThread.joinable()) {
		listeningThread.join();
	}

	if(not completeDisconnect) {
		fillErrorAggregatorWithNotAckedStatuses();
	} else {
		for(auto &[moduleNumber, errorAggregator]: errorAggregators_) {
			errorAggregator.destroy_error_aggregator();
		}
	}
}

int ExternalConnection::handleCommand(const ExternalProtocol::Command &commandMessage) {
	auto messageCounter = getCommandCounter(commandMessage);

	if(commandMessage.sessionid() != sessionId_) {
		log::logError("Command {} has incorrect sessionId", messageCounter);
		return COMMAND_INVALID;
	}
	if(serverMessageCounter_ != 0) {
		if(serverMessageCounter_ + 1 != messageCounter) {
			log::logError("Command {} is out of order", messageCounter);
			return COMMAND_INVALID;
		}
	}
	serverMessageCounter_ = messageCounter;

	ExternalProtocol::CommandResponse::Type responseType {};
	const auto deviceId = structures::DeviceIdentification(commandMessage.devicecommand().device());
	const auto &moduleNumber = deviceId.getModule();

	const auto it = errorAggregators_.find(moduleNumber);
	if (it == errorAggregators_.end()) {
		log::logError("Module with module number {} does not exist", moduleNumber);
		return NOT_OK;
	}

	const auto &errorAggregator = it->second;
	if(sentMessagesHandler_->isDeviceConnected(deviceId)) {
		responseType = ExternalProtocol::CommandResponse_Type_OK;
		commandQueue_->pushAndNotify(commandMessage.devicecommand());
	} else if(errorAggregator.is_device_type_supported(deviceId.getDeviceType()) != OK) {
		responseType = ExternalProtocol::CommandResponse_Type_DEVICE_NOT_SUPPORTED;
	} else {
		responseType = ExternalProtocol::CommandResponse_Type_DEVICE_NOT_CONNECTED;
	}

	auto commandResponse = common_utils::ProtobufUtils::createExternalClientCommandResponse(sessionId_, responseType,
																							messageCounter);
	log::logDebug("Sending command response with type={} and messageCounter={}", static_cast<int>(responseType), messageCounter);
	communicationChannel_->sendMessage(&commandResponse);
	return OK;
}

void ExternalConnection::receivingHandlerLoop() {
	stopReceiving.exchange(false);
	while(not stopReceiving) {
		if(state_.load() == ConnectionState::CONNECTING) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		const auto serverMessage = communicationChannel_->receiveMessage();
		if(serverMessage == nullptr || state_.load() == ConnectionState::NOT_CONNECTED) {
			continue;
		}
		if(serverMessage->has_command()) {
			const auto &command = serverMessage->command();
			log::logDebug("Handling COMMAND messageCounter={}", command.messagecounter());
			if(handleCommand(command) != OK) {
				reconnectQueue_->push(structures::ReconnectQueueItem(std::ref(*this), true));
				return;
			}
		} else if(serverMessage->has_statusresponse()) {
			const auto &statusResponse = serverMessage->statusresponse();
			log::logDebug("Handling STATUS_RESPONSE messageCounter={}", statusResponse.messagecounter());
			if(sentMessagesHandler_->acknowledgeStatus(statusResponse) != OK) {
				reconnectQueue_->pushAndNotify(structures::ReconnectQueueItem(std::ref(*this), false));
				return;
			}
		} else {
			log::logError("Received message with unexpected type(connect response), closing connection");
			reconnectQueue_->push(structures::ReconnectQueueItem(std::ref(*this), true));
			return;
		}

	}
}

u_int32_t ExternalConnection::getCommandCounter(const ExternalProtocol::Command &command) {
	return command.messagecounter();
}

void ExternalConnection::fillErrorAggregatorWithNotAckedStatuses() {
	for(const auto &notAckedStatus: sentMessagesHandler_->getNotAckedStatuses()) {
		const auto &device = notAckedStatus->getDevice();

		const auto &statusData = notAckedStatus->getStatus().devicestatus().statusdata();
		auto statusBuffer = moduleLibrary_.moduleLibraryHandlers.at(device.module())->constructBuffer(
			statusData.size());
		if (!statusBuffer.isEmpty()) {
			common_utils::ProtobufUtils::copyStatusToBuffer(notAckedStatus->getStatus().devicestatus(), statusBuffer);
		}

		const auto aggIt = errorAggregators_.find(device.module());
		if(aggIt == errorAggregators_.end()) {
			log::logError("Not-acked status references unknown module {}", static_cast<int>(device.module()));
			continue;
		}
		auto deviceId = structures::DeviceIdentification(device);
		aggIt->second.add_status_to_error_aggregator(statusBuffer, deviceId);
	}
	sentMessagesHandler_->clearAll();
}

void ExternalConnection::fillErrorAggregator(const InternalProtocol::DeviceStatus &deviceStatus) {
	int moduleNum = deviceStatus.device().module();
	const auto it = errorAggregators_.find(moduleNum);
	if(it == errorAggregators_.end()) {
		log::logError("Module with module number {} does no exists", moduleNum);
		return;
	}
	fillErrorAggregatorWithNotAckedStatuses();
	const auto &statusData = deviceStatus.statusdata();
	const auto statusBuffer = moduleLibrary_.moduleLibraryHandlers.at(moduleNum)->constructBuffer(
		statusData.size());
	if (!statusBuffer.isEmpty()) {
		common_utils::ProtobufUtils::copyStatusToBuffer(deviceStatus, statusBuffer);
	}

	const auto deviceId = structures::DeviceIdentification(deviceStatus.device());
	auto &errorAggregator = it->second;
	errorAggregator.add_status_to_error_aggregator(statusBuffer, deviceId);
}

std::vector<structures::DeviceIdentification> ExternalConnection::forceAggregationOnAllDevices(const std::vector<structures::DeviceIdentification> &connectedDevices) {
	std::vector<structures::DeviceIdentification> forcedDevices {};
	for(const auto &device: connectedDevices) {
		modules::Buffer last_status {};
		if (errorAggregators_.at(device.getModule()).get_last_status(last_status, device) == OK){
			continue;
		}
		moduleLibrary_.statusAggregators.at(device.getModule())->force_aggregation_on_device(device);
		forcedDevices.push_back(device);
	}
	return forcedDevices;
}

std::vector<structures::DeviceIdentification> ExternalConnection::getAllConnectedDevices() const {
	std::vector<structures::DeviceIdentification> devices {};
	for(const auto &moduleNumber: settings_.modules) {
		std::list<structures::DeviceIdentification> unique_devices {};
		auto statusAggregatorItr = moduleLibrary_.statusAggregators.find(moduleNumber);

		if (statusAggregatorItr == moduleLibrary_.statusAggregators.end())
		{
			log::logWarning("Module {} is defined in external-connection endpoint but is not specified in module-paths",
							moduleNumber);
			continue;
		}

		const int ret = statusAggregatorItr->second->get_unique_devices(unique_devices);
		if(ret <= 0) {
			log::logWarning("Module {} does not have any connected devices", moduleNumber);
			continue;
		}

		for (auto &device: unique_devices){
			devices.emplace_back(device);
		}
	}

	return devices;
}

ConnectionState ExternalConnection::getState() const { return state_.load(); }

void ExternalConnection::setNotInitialized() {
	state_.exchange(ConnectionState::NOT_INITIALIZED);
}

bool ExternalConnection::isModuleSupported(int moduleNum) const {
	return errorAggregators_.contains(moduleNum);
}


}
