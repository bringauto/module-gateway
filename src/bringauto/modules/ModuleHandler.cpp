#include <bringauto/modules/ModuleHandler.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/common_utils/MemoryUtils.hpp>

#include <memory_management.h>



namespace bringauto::modules {

namespace ip = InternalProtocol;
using log = bringauto::logging::Logger;

void ModuleHandler::destroy() {
	while(not fromInternalQueue_->empty()) {
		auto &message = fromInternalQueue_->front();
		if(message.disconnected()) {
			auto deviceId = message.getDeviceId();
			common_utils::MemoryUtils::deallocateDeviceId(deviceId);
		}
		fromInternalQueue_->pop();
	}
	log::logInfo("Module handler stopped");
}

void ModuleHandler::run() {
	log::logInfo("Module handler started");
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
		if(statusAggregator->getTimeoutedMessageReady()){
			struct buffer unique_devices {};
			int ret = statusAggregator->get_unique_devices(&unique_devices);
			if (ret == NOT_OK) {
				log::logError("Could not get unique devices in checkTimeoutedMessages");
				return;
			}
			device_identification *devicesPointer = static_cast<device_identification *>(unique_devices.data);
			for (int i = 0; i < ret; i++){
				struct device_identification deviceId {
					.module = devicesPointer[i].module,
					.device_type = devicesPointer[i].device_type,
					.device_role = devicesPointer[i].device_role,
					.device_name = devicesPointer[i].device_name
				};
				while(true) {
					struct ::buffer aggregatedStatusBuffer {};
					int remainingMessages = statusAggregator->get_aggregated_status(&aggregatedStatusBuffer, deviceId);
					if(remainingMessages == NO_MESSAGE_AVAILABLE) {
						break;
					}
					auto device = structures::DeviceIdentification(deviceId);
					auto internalProtocolDevice = device.convertToIPDevice();
					auto statusMessage = common_utils::ProtobufUtils::createInternalClientStatusMessage(internalProtocolDevice,
																										aggregatedStatusBuffer);
					toExternalQueue_->pushAndNotify(structures::InternalClientMessage(false, statusMessage));
					log::logDebug("Module handler pushed timeouted aggregated status, number of aggregated statuses in queue {}",
								  toExternalQueue_->size());
					statusAggregator->moduleDeallocate(&aggregatedStatusBuffer);
				}
				deallocate(&devicesPointer[i].device_role);
				deallocate(&devicesPointer[i].device_name);
			}
			deallocate(&unique_devices);
			statusAggregator->unsetTimeoutedMessageReady();
		}
	}
}

void ModuleHandler::handleDisconnect(device_identification deviceId) {
	const auto &moduleNumber = deviceId.module;
	std::string deviceName { static_cast<char *>(deviceId.device_name.data), deviceId.device_name.size_in_bytes };
	auto &statusAggregators = moduleLibrary_.statusAggregators;

	if(not statusAggregators.contains(moduleNumber)) {
		log::logWarning("Module number: {} is not supported", moduleNumber);
		common_utils::MemoryUtils::deallocateDeviceId(deviceId);
		return;
	}

	auto &statusAggregator = statusAggregators.at(moduleNumber);
	if(statusAggregator->is_device_valid(deviceId) == NOT_OK) {
		common_utils::MemoryUtils::deallocateDeviceId(deviceId);
		return;
	}

	int ret = statusAggregator->force_aggregation_on_device(deviceId);
	if(ret < 1) {
		log::logWarning("Force aggregation failed on device: {} with error code: {}", deviceName, ret);
		return;
	}
	auto device = structures::DeviceIdentification(deviceId);
	auto internalProtocolDevice = device.convertToIPDevice();
	sendAggregatedStatus(deviceId, internalProtocolDevice, true);

	statusAggregator->remove_device(deviceId);

	log::logInfo("Device {} disconnects", deviceName);
	common_utils::MemoryUtils::deallocateDeviceId(deviceId);
}

void ModuleHandler::sendAggregatedStatus(const device_identification &deviceId, const ip::Device &device,
										 bool disconnected) {
	auto &statusAggregator = moduleLibrary_.statusAggregators.at(deviceId.module);
	struct ::buffer aggregatedStatusBuffer {};
	statusAggregator->get_aggregated_status(&aggregatedStatusBuffer, deviceId);
	auto statusMessage = common_utils::ProtobufUtils::createInternalClientStatusMessage(device,
																						aggregatedStatusBuffer);
	toExternalQueue_->pushAndNotify(structures::InternalClientMessage(disconnected, statusMessage));
	log::logDebug("Module handler pushed aggregated status, number of aggregated statuses in queue {}",
				  toExternalQueue_->size());
	statusAggregator->moduleDeallocate(&aggregatedStatusBuffer);
}

void ModuleHandler::handleConnect(const ip::DeviceConnect &connect) {
	const auto &device = connect.device();
	const auto &moduleNumber = device.module();
	const auto &deviceName = device.devicename();
	auto &statusAggregators = moduleLibrary_.statusAggregators;
	log::logInfo("Module handler received Connect message from device: {}", deviceName);

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

	struct ::device_identification deviceId = common_utils::ProtobufUtils::parseDevice(device);
	if(statusAggregator->is_device_valid(deviceId) == OK) {
		log::logInfo("Device {} is replaced with device with higher priority", deviceName);
	}
	common_utils::MemoryUtils::deallocateDeviceId(deviceId);

	sendConnectResponse(device, ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_OK);
}

void
ModuleHandler::sendConnectResponse(const ip::Device &device, ip::DeviceConnectResponse_ResponseType response_type) {
	auto response = common_utils::ProtobufUtils::createInternalServerConnectResponseMessage(device, response_type);
	toInternalQueue_->pushAndNotify(response);
	log::logInfo("New device {} is trying to connect, sending response {}", device.devicename(), response_type);
}

void ModuleHandler::handleStatus(const ip::DeviceStatus &status) {
	const auto &device = status.device();
	const auto &moduleNumber = device.module();
	const auto &deviceName = device.devicename();
	auto &statusAggregators = moduleLibrary_.statusAggregators;
	log::logDebug("Module handler received status from device: {}", deviceName);

	if(not statusAggregators.contains(moduleNumber)) {
		log::logWarning("Module number: {} is not supported", moduleNumber);
		return;
	}
	auto &statusAggregator = statusAggregators[moduleNumber];

	struct ::buffer statusBuffer {};
	const auto &statusData = status.statusdata();
	if(statusAggregator->moduleAllocate(&statusBuffer, statusData.size()) == NOT_OK) {
		log::logError("Could not allocate memory for status message");
		return;
	}
	std::memcpy(statusBuffer.data, statusData.c_str(), statusData.size());

	struct ::device_identification deviceId = common_utils::ProtobufUtils::parseDevice(device);

	struct ::buffer commandBuffer {};
	int getCommandRc = statusAggregator->get_command(statusBuffer, deviceId, &commandBuffer);
	if(getCommandRc == OK) {
		auto deviceCommandMessage = common_utils::ProtobufUtils::createInternalServerCommandMessage(device,
																									commandBuffer);
		toInternalQueue_->pushAndNotify(deviceCommandMessage);
		log::logDebug("Module handler succesfully retrieved command and sent it to device: {}", deviceName);
		statusAggregator->moduleDeallocate(&commandBuffer);
	} else {
		log::logWarning("Retrieving command failed with return code: {}", getCommandRc);
		common_utils::MemoryUtils::deallocateDeviceId(deviceId);
		statusAggregator->moduleDeallocate(&statusBuffer);
		return;
	}

	int addStatusToAggregatorRc = statusAggregator->add_status_to_aggregator(statusBuffer, deviceId);
	if(addStatusToAggregatorRc < 0) {
		log::logWarning("Add status to aggregator failed with return code: {}", addStatusToAggregatorRc);
		common_utils::MemoryUtils::deallocateDeviceId(deviceId);
		statusAggregator->moduleDeallocate(&statusBuffer);
		return;
	}

	while(addStatusToAggregatorRc > 0) {
		sendAggregatedStatus(deviceId, device, false);
		addStatusToAggregatorRc--;
	}

	common_utils::MemoryUtils::deallocateDeviceId(deviceId);
	statusAggregator->moduleDeallocate(&statusBuffer);
}

}
