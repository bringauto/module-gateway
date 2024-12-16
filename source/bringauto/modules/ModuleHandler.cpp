#include <bringauto/modules/ModuleHandler.hpp>
#include <bringauto/settings/LoggerId.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>

#include <fleet_protocol/common_headers/memory_management.h>



namespace bringauto::modules {

namespace ip = InternalProtocol;

void ModuleHandler::destroy() {
	while(not fromInternalQueue_->empty()) {
		auto &message = fromInternalQueue_->front();
		if(message.disconnected()) {
			auto deviceId = message.getDeviceId();
		}
		fromInternalQueue_->pop();
	}
	settings::Logger::logInfo("Module handler stopped");
}

void ModuleHandler::run() {
	settings::Logger::logInfo("Module handler started, constants used: queue_timeout_length: {}, status_aggregation_timeout: {}",
				 settings::queue_timeout_length.count(), settings::status_aggregation_timeout.count());
	handleMessages();
}

void ModuleHandler::handleMessages() {
	while(not context_->ioContext.stopped()) {
		if(fromInternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			checkTimeoutedMessages();
			continue;
		}
		checkTimeoutedMessages();

		auto &message = fromInternalQueue_->front();
		if(message.disconnected()) {
			handleDisconnect(message.getDeviceId());
		} else if(message.getMessage().has_deviceconnect()) {
			handleConnect(message.getMessage().deviceconnect());
		} else if(message.getMessage().has_devicestatus()) {
			handleStatus(message.getMessage().devicestatus());
		}
		fromInternalQueue_->pop();
	}
}

void ModuleHandler::checkTimeoutedMessages(){
	for (const auto& [key, statusAggregator] : moduleLibrary_.statusAggregators) {
		auto moduleLibraryHandler = moduleLibrary_.moduleLibraryHandlers.at(key);
		if(statusAggregator->getTimeoutedMessageReady()){
			std::list<structures::DeviceIdentification> unique_devices {};
			int ret = statusAggregator->get_unique_devices(unique_devices);
			if (ret == NOT_OK) {
				settings::Logger::logError("Could not get unique devices in checkTimeoutedMessages");
				return;
			}
			
			for (auto &device: unique_devices) {
				while(true) {
					Buffer aggregatedStatusBuffer {};
					int remainingMessages = statusAggregator->get_aggregated_status(aggregatedStatusBuffer, device);
					if(remainingMessages < 0) {
						break;
					}
					auto internalProtocolDevice = device.convertToIPDevice();
					auto statusMessage = common_utils::ProtobufUtils::createInternalClientStatusMessage(internalProtocolDevice,
																										aggregatedStatusBuffer);
					toExternalQueue_->pushAndNotify(structures::InternalClientMessage(false, statusMessage));
					settings::Logger::logDebug("Module handler pushed a timed out aggregated status, number of aggregated statuses in queue {}",
								  toExternalQueue_->size());
					checkExternalQueueSize();
				}

				if(statusAggregator->getDeviceTimeoutCount(device) >= settings::status_aggregation_timeout_max_count){
					settings::Logger::logWarning("Device {} not sending statuses for too long, disconnecting it", device.convertToString());
					toInternalQueue_->pushAndNotify(structures::ModuleHandlerMessage(device));
				}
			}
			statusAggregator->unsetTimeoutedMessageReady();
		}
	}
}

void ModuleHandler::handleDisconnect(const structures::DeviceIdentification& deviceId) {
	const auto &moduleNumber = deviceId.getModule();
	const std::string& deviceName { deviceId.getDeviceName() };
	auto &statusAggregators = moduleLibrary_.statusAggregators;

	if(not statusAggregators.contains(moduleNumber)) {
		settings::Logger::logWarning("Module number: {} is not supported", moduleNumber);
		return;
	}

	auto &statusAggregator = statusAggregators.at(moduleNumber);
	if(statusAggregator->is_device_valid(deviceId) == NOT_OK) {
		settings::Logger::logWarning("Trying to disconnect invalid device");
		return;
	}

	int ret = statusAggregator->force_aggregation_on_device(deviceId);
	if(ret < 1) {
		settings::Logger::logWarning("Force aggregation failed on device: {} with error code: {}", deviceName, ret);
		return;
	}
	auto device = structures::DeviceIdentification(deviceId);
	auto internalProtocolDevice = device.convertToIPDevice();
	sendAggregatedStatus(deviceId, internalProtocolDevice, true);

	statusAggregator->remove_device(deviceId);

	settings::Logger::logInfo("Device {} disconnects", deviceName);
}

void ModuleHandler::sendAggregatedStatus(const structures::DeviceIdentification &deviceId, const ip::Device &device,
										 bool disconnected) {
	const auto &statusAggregator = moduleLibrary_.statusAggregators.at(deviceId.getModule());
	Buffer aggregatedStatusBuffer {};
	statusAggregator->get_aggregated_status(aggregatedStatusBuffer, deviceId);
	const auto statusMessage = common_utils::ProtobufUtils::createInternalClientStatusMessage(device, aggregatedStatusBuffer);
	toExternalQueue_->pushAndNotify(structures::InternalClientMessage(disconnected, statusMessage));
	settings::Logger::logDebug("Module handler pushed aggregated status, number of aggregated statuses in queue {}",
				  toExternalQueue_->size());
	checkExternalQueueSize();
}

void ModuleHandler::handleConnect(const ip::DeviceConnect &connect) {
	const auto &device = connect.device();
	const auto &moduleNumber = device.module();
	const auto &deviceName = device.devicename();
	auto &statusAggregators = moduleLibrary_.statusAggregators;
	settings::Logger::logInfo("Module handler received Connect message from device: {}", deviceName);

	if(not statusAggregators.contains(moduleNumber)) {
		sendConnectResponse(device,
							ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_MODULE_NOT_SUPPORTED);
		return;
	}

	auto &statusAggregator = statusAggregators.at(moduleNumber);
	if(statusAggregator->is_device_type_supported(device.devicetype()) == NOT_OK) {
		sendConnectResponse(device,
							ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_DEVICE_NOT_SUPPORTED);
		return;
	}

	auto deviceId = structures::DeviceIdentification(device);
	if(statusAggregator->is_device_valid(deviceId) == OK) {
		settings::Logger::logInfo("Device {} is replaced by device with higher priority", deviceName);
	}

	sendConnectResponse(device, ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_OK);
}

void
ModuleHandler::sendConnectResponse(const ip::Device &device, ip::DeviceConnectResponse_ResponseType response_type) {
	auto response = common_utils::ProtobufUtils::createInternalServerConnectResponseMessage(device, response_type);
	toInternalQueue_->pushAndNotify(structures::ModuleHandlerMessage(false,response));
	settings::Logger::logInfo("New device {} is trying to connect, sending response {}", device.devicename(), static_cast<int>(response_type));
}

void ModuleHandler::handleStatus(const ip::DeviceStatus &status) {
	const auto &device = status.device();
	const auto &moduleNumber = device.module();
	const auto &deviceName = device.devicename();
	auto &statusAggregators = moduleLibrary_.statusAggregators;
	settings::Logger::logDebug("Module handler received status from device: {}", deviceName);

	if(not statusAggregators.contains(moduleNumber)) {
		settings::Logger::logWarning("Module number: {} is not supported", static_cast<int>(moduleNumber));
		return;
	}
	auto statusAggregator = statusAggregators[moduleNumber];
	const auto moduleHandler = moduleLibrary_.moduleLibraryHandlers.at(moduleNumber);

	const auto &statusData = status.statusdata();
	auto statusBuffer = moduleHandler->constructBuffer(statusData.size());
	if (!statusBuffer.isEmpty()) {
		common_utils::ProtobufUtils::copyStatusToBuffer(status, statusBuffer);
	}

	const auto deviceId = structures::DeviceIdentification(device);

	if(moduleHandler->statusDataValid(statusBuffer, deviceId.getDeviceType()) == NOT_OK) {
		settings::Logger::logWarning("Invalid status data on device id: {}", deviceId.convertToString());
		return;
	}

	int addStatusToAggregatorRc = statusAggregator->add_status_to_aggregator(statusBuffer, deviceId);
	if(addStatusToAggregatorRc < 0) {
		settings::Logger::logWarning("Add status to aggregator failed with return code: {}", addStatusToAggregatorRc);
		return;
	}
	
	Buffer commandBuffer {};
	int getCommandRc = statusAggregator->get_command(statusBuffer, deviceId, commandBuffer);
	if(getCommandRc == OK) {
		auto deviceCommandMessage = common_utils::ProtobufUtils::createInternalServerCommandMessage(device,
																									commandBuffer);
		toInternalQueue_->pushAndNotify(structures::ModuleHandlerMessage(false, deviceCommandMessage));
		settings::Logger::logDebug("Module handler successfully retrieved command and sent it to device: {}", deviceName);
	} else {
		settings::Logger::logWarning("Retrieving command failed with return code: {}", getCommandRc);
		return;
	}

	while(addStatusToAggregatorRc > 0) {
		sendAggregatedStatus(deviceId, device, false);
		addStatusToAggregatorRc--;
	}
}

void ModuleHandler::checkExternalQueueSize() {
	if(toExternalQueue_->size() > settings::max_external_queue_size) {
		settings::Logger::logError("External queue size is too big, external client is not handling messages");
		destroy();
		throw std::runtime_error("External queue size is too big");
	}
}

}
