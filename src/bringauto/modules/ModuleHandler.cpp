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
			continue;
		}

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

void ModuleHandler::handleDisconnect(device_identification deviceId) {
	const auto &moduleNumber = deviceId.module;
	std::string deviceName {static_cast<char *>(deviceId.device_name.data), deviceId.device_name.size_in_bytes};
	auto &statusAggregators = moduleLibrary_.statusAggregators;

	if(not statusAggregators.contains(moduleNumber)) {
		log::logWarning("Module number: {} is not supported", moduleNumber);
		common_utils::MemoryUtils::deallocateDeviceId(deviceId);
		return;
	}

	auto &statusAggregator = statusAggregators.at(moduleNumber);
	if (statusAggregator->is_device_valid(deviceId) == NOT_OK){
		common_utils::MemoryUtils::deallocateDeviceId(deviceId);
		return;
	}

	statusAggregator->force_aggregation_on_device(deviceId);
	auto device = common_utils::ProtobufUtils::createDevice(deviceId);

	//create function probably
	struct ::buffer aggregatedStatusBuffer {};
	statusAggregator->get_aggregated_status(&aggregatedStatusBuffer, deviceId);
	auto statusMessage = common_utils::ProtobufUtils::createInternalClientStatusMessage(device,
																						aggregatedStatusBuffer);
	toExternalQueue_->pushAndNotify(structures::InternalClientMessage(true, statusMessage));
	log::logDebug("Module handler pushed aggregated status, number of aggregated statuses in queue {}",
				  toExternalQueue_->size());
	deallocate(&aggregatedStatusBuffer);

	statusAggregator->remove_device(deviceId);
	log::logCritical("Device {} disconnects", deviceName);
	common_utils::MemoryUtils::deallocateDeviceId(deviceId);
}

void ModuleHandler::handleConnect(const ip::DeviceConnect &connect) {
	const auto &device = connect.device();
	const auto &moduleNumber = device.module();
	const auto &deviceName = device.devicename();
	auto &statusAggregators = moduleLibrary_.statusAggregators;
	log::logInfo("Module handler received Connect message from device: {}", deviceName);

	if(not statusAggregators.contains(moduleNumber)) {
		sendConnectResponse(device, ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_MODULE_NOT_SUPPORTED);
		return;
	}

	auto &statusAggregator = statusAggregators.at(moduleNumber);
	if(statusAggregator->is_device_type_supported(device.devicetype()) == NOT_OK) {
		sendConnectResponse(device, ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_DEVICE_NOT_SUPPORTED);
		return;
	}

	struct ::device_identification deviceId = common_utils::ProtobufUtils::parseDevice(device);
	if (statusAggregator->is_device_valid(deviceId) == OK){
		log::logInfo("Device {} is replaced with device with higher priority", deviceName);
	}
	common_utils::MemoryUtils::deallocateDeviceId(deviceId);

	sendConnectResponse(device, ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_OK);
}

void ModuleHandler::sendConnectResponse(const ip::Device &device, ip::DeviceConnectResponse_ResponseType response_type){
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

	struct ::buffer statusBuffer {};
	const auto &statusData = status.statusdata();
	if(allocate(&statusBuffer, statusData.size()) == NOT_OK) {
		log::logError("Could not allocate memory for status message");
		return;
	}
	std::memcpy(statusBuffer.data, statusData.c_str(), statusData.size());

	struct ::device_identification deviceId = common_utils::ProtobufUtils::parseDevice(device);

	auto &statusAggregator = statusAggregators[moduleNumber];
	int addStatusToAggregatorRc = statusAggregator->add_status_to_aggregator(statusBuffer, deviceId);
	if(addStatusToAggregatorRc < 0) {
		log::logWarning("Add status to aggregator failed with return code: {}", addStatusToAggregatorRc);
	}

	struct ::buffer commandBuffer {};
	int getCommandrRc = statusAggregator->get_command(statusBuffer, deviceId, &commandBuffer);
	if(getCommandrRc == OK) {
		auto deviceCommandMessage = common_utils::ProtobufUtils::createInternalServerCommandMessage(device, commandBuffer);
		toInternalQueue_->pushAndNotify(deviceCommandMessage);
		log::logDebug("Module handler succesfully retrieved command and sent it to device: {}", deviceName);
		deallocate(&commandBuffer);
	} else {
		log::logWarning("Retrieving command failed with return code: {}", getCommandrRc);
	}

	while(addStatusToAggregatorRc > 0) {
		struct ::buffer aggregatedStatusBuffer {};
		statusAggregator->get_aggregated_status(&aggregatedStatusBuffer, deviceId);
		auto statusMessage = common_utils::ProtobufUtils::createInternalClientStatusMessage(device,
																							aggregatedStatusBuffer);
		toExternalQueue_->pushAndNotify(structures::InternalClientMessage(false, statusMessage));
		log::logDebug("Module handler pushed aggregated status, number of aggregated statuses in queue {}",
					  toExternalQueue_->size());
		deallocate(&aggregatedStatusBuffer);
		addStatusToAggregatorRc--;
	}

	common_utils::MemoryUtils::deallocateDeviceId(deviceId);
	deallocate(&statusBuffer);
}

}
