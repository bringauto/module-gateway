#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/common_utils/StringUtils.hpp>
#include <bringauto/common_utils/MemoryUtils.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>

#include <bringauto/logging/Logger.hpp>

#include <random>



namespace bringauto::external_client::connection {
using log = bringauto::logging::Logger;

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

void ExternalConnection::init(const std::string &company, const std::string &vehicleName) {
	for(const auto &moduleNum: settings_.modules) {
		errorAggregators[moduleNum] = ErrorAggregator();
		errorAggregators[moduleNum].init_error_aggregator(moduleLibrary_.moduleLibraryHandlers[moduleNum]);
	}
	switch(settings_.protocolType) {
		case structures::ProtocolType::MQTT:
			communicationChannel_ = std::make_unique<communication::MqttCommunication>(settings_);
			communicationChannel_->setProperties(company, vehicleName);
			break;
		case structures::ProtocolType::INVALID:
		default:
			break;
	}
}

void ExternalConnection::sendStatus(const InternalProtocol::DeviceStatus &status,
									ExternalProtocol::Status::DeviceState deviceState,
									const buffer &errorMessage) {
	const auto &device = status.device();

	if(errorAggregators.find(device.module()) == errorAggregators.end()) {
		log::logError(
				"Status with module number ({}) was passed to external connection, that doesn't support this module",
				device.module());
	}

	auto deviceId = common_utils::ProtobufUtils::parseDevice(device);
	std::string id = common_utils::ProtobufUtils::getId(deviceId);
	common_utils::MemoryUtils::deallocateDeviceId(deviceId);

	switch(deviceState) {
		case ExternalProtocol::Status_DeviceState_CONNECTING:
			sentMessagesHandler_->addDeviceAsConnected(id);
			break;
		case ExternalProtocol::Status_DeviceState_RUNNING:
			if(not sentMessagesHandler_->isDeviceConnected(id)) {
				deviceState = ExternalProtocol::Status_DeviceState_CONNECTING;
				sentMessagesHandler_->addDeviceAsConnected(id);
			}
			break;
		case ExternalProtocol::Status_DeviceState_DISCONNECT:
			sentMessagesHandler_->deleteConnectedDevice(id);
			break;
		case ExternalProtocol::Status_DeviceState_ERROR:
		default:
			// TODO What does ERROR mean and what should happen?
			break;
	}

	auto externalMessage = common_utils::ProtobufUtils::createExternalClientStatus(sessionId_,
																				   deviceState,
																				   getNextStatusCounter(),
																				   status,
																				   errorMessage);
	sentMessagesHandler_->addNotAckedStatus(externalMessage.status());
	try {
		communicationChannel_->sendMessage(&externalMessage);
	} catch(std::runtime_error &e) {
		log::logError(e.what());
		endConnection(false);
	}
	log::logDebug("Sending status with messageCounter '{}' with aggregated errorMessage: {}", clientMessageCounter_,
				  errorMessage.size_in_bytes > 0 ? errorMessage.data : "");
}

int ExternalConnection::initializeConnection() {
	if(state_.load() == ConnectionState::NOT_INITIALIZED) {
		state_.exchange(ConnectionState::NOT_CONNECTED);
	}

	try {
		communicationChannel_->initializeConnection();
	} catch(std::exception &e) {
		log::logError("Unable to create connection to {}:{} reason: {}", settings_.serverIp, settings_.port, e.what());
		return -1;
	}
	log::logInfo("Initializing connection to endpoint {}:{}", settings_.serverIp, settings_.port);

	std::vector<structures::DeviceIdentification> devices = getAllConnectedDevices();

	state_.exchange(ConnectionState::CONNECTING);
	log::logInfo("Connect sequence: 1st step (sending list of devices)");
	if(connectMessageHandle(devices) != 0) {
		log::logError("Connect sequence to server {}:{}, failed in 1st step", settings_.serverIp, settings_.port);
		state_.exchange(ConnectionState::NOT_CONNECTED);
		return -1;
	}
	log::logInfo("Connect sequence: 2nd step (sending statuses of all connected devices)");
	if(statusMessageHandle(devices) != 0) {
		log::logError("Connect sequence to server {}:{}, failed in 2nd step", settings_.serverIp, settings_.port);
		state_.exchange(ConnectionState::NOT_CONNECTED);
		return -1;
	}
	log::logInfo("Connect sequence: 3rd step (receiving commands for devices in previous step)");
	if(commandMessageHandle(devices) != 0) {
		log::logError("Connect sequence to server {}:{}, failed in 3rd step", settings_.serverIp, settings_.port);
		state_.exchange(ConnectionState::NOT_CONNECTED);
		return -1;
	}
	listeningThread = std::jthread(&ExternalConnection::receivingHandlerLoop, this);
	state_.exchange(ConnectionState::CONNECTED);
	for(auto &[moduleNum, errorAggregator]: errorAggregators) {
		errorAggregator.clear_error_aggregator();
	}
	log::logInfo("Connect sequence successful. Server {}:{}", settings_.serverIp, settings_.port);
	return 0;
}

int ExternalConnection::connectMessageHandle(const std::vector<structures::DeviceIdentification> &devices) {
	setSessionId();

	auto connectMessage = common_utils::ProtobufUtils::createExternalClientConnect(sessionId_, company_, vehicleName_,
																				   devices);
	communicationChannel_->sendMessage(&connectMessage);

	const auto connectResponseMsg = communicationChannel_->receiveMessage();
	if(connectResponseMsg == nullptr) {
		log::logError("Communication client couldn't receive any message");
		return -1;
	}
	if(not connectResponseMsg->has_connectresponse()) {
		log::logError("Received message has not type connect response");
		return -1;
	}
	if(connectResponseMsg->connectresponse().sessionid() != sessionId_) {
		log::logError("Bad session id in connect response");
		return -1;
	}
	if(connectResponseMsg->connectresponse().type() == ExternalProtocol::ConnectResponse_Type_ALREADY_LOGGED) {
		log::logError("Already logged in");
		return -1;
	}
	return 0;
}

int ExternalConnection::statusMessageHandle(const std::vector<structures::DeviceIdentification> &devices) {
	for(const auto &deviceIdentification: devices) {
		auto device = deviceIdentification.convertToCStruct();
		const int &deviceModule = device.module;
		struct buffer errorBuffer {};
		struct buffer statusBuffer {};

		const auto &lastErrorStatusRc = errorAggregators[deviceModule].get_error(&errorBuffer, device);
		if(lastErrorStatusRc == DEVICE_NOT_REGISTERED) {
			logging::Logger::logError("Device is not registered in error aggregator: {} {}",
									  std::string { static_cast<char *>(device.device_role.data),
													device.device_role.size_in_bytes },
									  std::string { static_cast<char *>(device.device_name.data),
													device.device_name.size_in_bytes });
			return -1;
		}

		int lastStatusRc = errorAggregators[deviceModule].get_last_status(&statusBuffer, device);
		if(lastStatusRc != OK) {
			logging::Logger::logError("Cannot obtain status for device: {} {}",
									  std::string { static_cast<char *>(device.device_role.data),
													device.device_role.size_in_bytes },
									  std::string { static_cast<char *>(device.device_name.data),
													device.device_name.size_in_bytes });
			return -1;
		}
		auto deviceStatus = common_utils::ProtobufUtils::createDeviceStatus(device, statusBuffer);
		sendStatus(deviceStatus, ExternalProtocol::Status_DeviceState_CONNECTING, errorBuffer);

		common_utils::MemoryUtils::deallocateDeviceId(device);
	}
	for(int i = 0; i < devices.size(); ++i) {
		const auto statusResponseMsg = communicationChannel_->receiveMessage();
		if(statusResponseMsg == nullptr) {
			log::logError("Communication client couldn't receive any message");
			return -1;
		}
		if(not statusResponseMsg->has_statusresponse()) {
			log::logError("Received message has not type status response");
			return -1;
		}
		if(statusResponseMsg->statusresponse().type() != ExternalProtocol::StatusResponse_Type_OK) {
			log::logError("Status response does not contain OK");
			return -1;
		}
		if(statusResponseMsg->statusresponse().sessionid() != sessionId_) {
			log::logError("Bad session id in status response");
			return -1;
		}
		sentMessagesHandler_->acknowledgeStatus(statusResponseMsg->statusresponse());
	}
	return 0;
}

int ExternalConnection::commandMessageHandle(const std::vector<structures::DeviceIdentification> &devices) {
	for(int i = 0; i < devices.size(); ++i) {
		const auto commandMsg = communicationChannel_->receiveMessage();
		if(commandMsg == nullptr) {
			log::logError("Communication client couldn't receive any message");
			return -1;
		}
		if(not commandMsg->has_command()) {
			log::logError("Received message has not type command");
			return -1;
		}
		if(handleCommand(commandMsg->command()) != 0) {
			return -1;
		}
	}
	return 0;
}

void ExternalConnection::setSessionId() {
	static auto &chrs = "0123456789"
						"abcdefghijklmnopqrstuvwxyz"
						"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	thread_local
	static std::mt19937 rg { std::random_device {}() };
	thread_local
	static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

	for(int i = 0; i < KEY_LENGHT; i++) {
		sessionId_ += chrs[pick(rg)];
	}
	sessionId_ = carId_ + sessionId_;
}

u_int32_t ExternalConnection::getNextStatusCounter() {
	return ++clientMessageCounter_;
}

void ExternalConnection::endConnection(bool completeDisconnect = false) {
	state_.exchange(ConnectionState::NOT_CONNECTED);
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
		for(auto &[moduleNumber, errorAggregator]: errorAggregators) {
			errorAggregator.destroy_error_aggregator();
		}
	}
}

int ExternalConnection::handleCommand(const ExternalProtocol::Command &commandMessage) {
	auto messageCounter = getCommandCounter(commandMessage);

	if(commandMessage.sessionid() != sessionId_) {
		log::logError("Command {} has incorrect sessionId", messageCounter);
		return -2;
	}
	if(serverMessageCounter_ != 0) {
		if(serverMessageCounter_ + 1 != messageCounter) {
			log::logError("Command {} is out of order", messageCounter);
			return -1; // Out of order
		}
	}
	serverMessageCounter_ = messageCounter;

	ExternalProtocol::CommandResponse::Type responseType;
	auto deviceId = common_utils::ProtobufUtils::parseDevice(commandMessage.devicecommand().device());
	auto id = common_utils::ProtobufUtils::getId(deviceId);
	auto &errorAggregator = errorAggregators.at(deviceId.module);
	if(sentMessagesHandler_->isDeviceConnected(id)) {
		responseType = ExternalProtocol::CommandResponse_Type_OK;
		commandQueue_->pushAndNotify(commandMessage.devicecommand());
	} else if(errorAggregator.is_device_type_supported(deviceId.device_type) == NOT_OK) {
		responseType = ExternalProtocol::CommandResponse_Type_DEVICE_NOT_SUPPORTED;
	} else {
		responseType = ExternalProtocol::CommandResponse_Type_DEVICE_NOT_CONNECTED;
	}
	common_utils::MemoryUtils::deallocateDeviceId(deviceId);

	auto commandResponse = common_utils::ProtobufUtils::createExternalClientCommandResponse(sessionId_, responseType,
																							messageCounter);
	log::logDebug("Sending command response with type={} and messageCounter={}", responseType, messageCounter);
	communicationChannel_->sendMessage(&commandResponse);
	return 0;
}

bool ExternalConnection::hasAnyDeviceConnected() {
	return sentMessagesHandler_->isAnyDeviceConnected();
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
			if(handleCommand(command) != 0) {
				reconnectQueue_->push(structures::ReconnectQueueItem(std::ref(*this), true));
				return;
			}
		} else if(serverMessage->has_statusresponse()) {
			const auto &statusResponse = serverMessage->statusresponse();
			log::logDebug("Handling STATUS_RESPONSE messageCounter={}", statusResponse.messagecounter());
			if(sentMessagesHandler_->acknowledgeStatus(statusResponse) != OK) {
				reconnectQueue_->push(structures::ReconnectQueueItem(std::ref(*this), false));
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

		struct ::buffer statusBuffer {};
		const auto &statusData = notAckedStatus->getStatus().devicestatus().statusdata();
		if(allocate(&statusBuffer, statusData.size()) == NOT_OK) {
			log::logError("Could not allocate memory for status message");
			return;
		}
		std::memcpy(statusBuffer.data, statusData.c_str(), statusData.size());

		auto deviceId = common_utils::ProtobufUtils::parseDevice(device);
		errorAggregators[device.module()].add_status_to_error_aggregator(statusBuffer, deviceId);
		common_utils::MemoryUtils::deallocateDeviceId(deviceId);
		deallocate(&statusBuffer);
	}
	sentMessagesHandler_->clearAll();
}

void ExternalConnection::fillErrorAggregator(const InternalProtocol::DeviceStatus &deviceStatus) {
	fillErrorAggregatorWithNotAckedStatuses();
	int moduleNum = deviceStatus.device().module();
	if(errorAggregators.find(moduleNum) != errorAggregators.end()) {
		struct ::buffer statusBuffer {};
		const auto &statusData = deviceStatus.statusdata();
		if(allocate(&statusBuffer, statusData.size()) == NOT_OK) {
			log::logError("Could not allocate memory for status message");
			return;
		}
		std::memcpy(statusBuffer.data, statusData.c_str(), statusData.size());

		auto deviceId = common_utils::ProtobufUtils::parseDevice(deviceStatus.device());
		errorAggregators[moduleNum].add_status_to_error_aggregator(statusBuffer, deviceId);
		common_utils::MemoryUtils::deallocateDeviceId(deviceId);
		deallocate(&statusBuffer);
	} else {
		log::logError("Device status with unsupported module was passed to fillErrorAggregator()");
	}
}

int ExternalConnection::forceAggregationOnAllDevices() {
	auto devices = getAllConnectedDevices();
	for(const auto &device: devices) {
		auto deviceId = device.convertToCStruct();
		moduleLibrary_.statusAggregators.at(device.getModule())->force_aggregation_on_device(deviceId);
		common_utils::MemoryUtils::deallocateDeviceId(deviceId);
	}
	return devices.size();
}

std::vector<structures::DeviceIdentification> ExternalConnection::getAllConnectedDevices() {
	std::vector<structures::DeviceIdentification> devices {};
	for(const auto &moduleNumber: settings_.modules) {
		struct buffer unique_devices {};
		int ret = moduleLibrary_.statusAggregators.at(moduleNumber)->get_unique_devices(&unique_devices);
		if(ret <= 0) {
			log::logWarning("Module {} does not have any connected devices", moduleNumber);
			deallocate(&unique_devices);
			continue;
		}
		std::string devicesString { static_cast<char *>(unique_devices.data), unique_devices.size_in_bytes };
		deallocate(&unique_devices);
		auto devicesVec = common_utils::StringUtils::splitString(devicesString, ',');
		for(const auto &device: devicesVec) {
			devices.emplace_back(device);
		}
	}

	return devices;
}

ConnectionState ExternalConnection::getState() const { return state_.load(); }

void ExternalConnection::setNotInitialized() {
	state_.exchange(ConnectionState::NOT_INITIALIZED);
}

bool ExternalConnection::isModuleSupported(int moduleNum) {
	return errorAggregators.find(moduleNum) != errorAggregators.end();
}


}
