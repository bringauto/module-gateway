#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/common_utils/MemoryUtils.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>

#include <dlfcn.h>



namespace bringauto::modules {

using log = bringauto::logging::Logger;

void StatusAggregator::clear_device(const std::string &key) {
	auto &deviceState = devices.at(key);
	auto &aggregatedMessages = deviceState.getAggregatedMessages();
	while(not aggregatedMessages.empty()) {
		auto message = aggregatedMessages.front();
		if(message.data != nullptr) {
			deallocate(&message);
		}
		aggregatedMessages.pop();
	}
	deviceState.deallocateStatus();
	std::lock_guard<std::mutex> lock(mutex_);
	deallocate(&deviceState.command);
}

void
StatusAggregator::aggregateStatus(structures::StatusAggregatorDeviceState &deviceState, const buffer &status,
								  const unsigned int &device_type) {
	auto &currStatus = deviceState.getStatus();
	struct buffer aggregatedStatusBuff {};
	module_->aggregateStatus(&aggregatedStatusBuff, currStatus, status, device_type);
	deviceState.setStatusAndResetTimer(aggregatedStatusBuff);
}

int StatusAggregator::init_status_aggregator() {
	return OK;
}

int StatusAggregator::destroy_status_aggregator() {
	clear_all_devices();
	return OK;
}

int StatusAggregator::clear_all_devices() {
	for(auto &[key, device]: devices) {
		clear_device(key);
	}
	return OK;
}

int StatusAggregator::clear_device(const struct ::device_identification device) {
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	std::string id = common_utils::ProtobufUtils::getId(device);
	clear_device(id);
	return OK;
}

int StatusAggregator::remove_device(const struct ::device_identification device) {
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	std::string id = common_utils::ProtobufUtils::getId(device);
	clear_device(device);
	devices.erase(id);
	return OK;
}

int StatusAggregator::add_status_to_aggregator(const struct ::buffer status,
											   const struct ::device_identification device) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}

	std::string id = common_utils::ProtobufUtils::getId(device);
	if(status.size_in_bytes == 0 || module_->statusDataValid(status, device_type) == NOT_OK) {
		log::logWarning("Invalid status data on device: {}", id);
		return NOT_OK;
	}

	if(not devices.contains(id)) {
		struct buffer commandBuffer {};
		module_->generateFirstCommand(&commandBuffer, device_type);
		struct buffer statusBuffer {};
		allocate(&statusBuffer, status.size_in_bytes);
		std::memcpy(statusBuffer.data, status.data, status.size_in_bytes);

		struct ::device_identification deviceId {};
		deviceId.module = device.module;
		deviceId.priority = device.priority;
		deviceId.device_type = device_type;
		common_utils::MemoryUtils::initBuffer(deviceId.device_name,
											  { static_cast<char *>(device.device_name.data),
												device.device_name.size_in_bytes });
		common_utils::MemoryUtils::initBuffer(deviceId.device_role,
											  { static_cast<char *>(device.device_role.data),
												device.device_role.size_in_bytes });

		std::function<int(struct ::device_identification)> fun = [&](
				struct ::device_identification deviceId) { return this->force_aggregation_on_device(deviceId); };
		devices.insert(
				{ id, structures::StatusAggregatorDeviceState(context_, fun, deviceId, commandBuffer, statusBuffer) });

		return 0;
	}

	auto &deviceState = devices.at(id);
	auto &currStatus = deviceState.getStatus();
	auto &aggregatedMessages = deviceState.getAggregatedMessages();
	if(module_->sendStatusCondition(currStatus, status, device_type) == OK) {
		aggregateStatus(deviceState, status, device_type);
		struct buffer statusToSendBuff {};
		allocate(&statusToSendBuff, currStatus.size_in_bytes);
		std::memcpy(statusToSendBuff.data, currStatus.data, currStatus.size_in_bytes);
		aggregatedMessages.push(statusToSendBuff);
	} else {
		struct buffer aggregatedStatusBuff {};
		module_->aggregateStatus(&aggregatedStatusBuff, currStatus, status, device_type);
		deviceState.setStatus(aggregatedStatusBuff);
	}

	return aggregatedMessages.size();
}

int StatusAggregator::get_aggregated_status(struct ::buffer *generated_status,
											const struct ::device_identification device) {
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}

	std::string id = common_utils::ProtobufUtils::getId(device);
	auto &aggregatedMessages = devices.at(id).getAggregatedMessages();
	if(aggregatedMessages.size() == 0) {
		return NO_MESSAGE_AVAILABLE;
	}

	auto &status = aggregatedMessages.front();
	generated_status->data = status.data;
	generated_status->size_in_bytes = status.size_in_bytes;
	aggregatedMessages.pop();
	return OK;
}

int StatusAggregator::get_unique_devices(struct ::buffer *unique_devices_buffer) {
	std::stringstream output {};
	for(auto const &[key, value]: devices) {
		output << key << ",";
	}
	std::string str = output.str();
	if(!str.empty()) {
		str.pop_back();
	}
	if(allocate(unique_devices_buffer, str.size()) == NOT_OK) {
		log::logError("Could not allocate buffer in get_unique_devices");
		return NOT_OK;
	}
	std::memcpy(unique_devices_buffer->data, str.c_str(), str.size());
	return devices.size();
}

int StatusAggregator::force_aggregation_on_device(const struct ::device_identification device) {
	std::string id = common_utils::ProtobufUtils::getId(device);
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}

	const auto &statusBuffer = devices.at(id).getStatus();
	struct buffer forcedStatusBuffer {};
	if(allocate(&forcedStatusBuffer, statusBuffer.size_in_bytes) == NOT_OK) {
		log::logError("Could not allocate buffer in force_aggregation_on_device");
		return NOT_OK;
	}

	std::memcpy(forcedStatusBuffer.data, statusBuffer.data, statusBuffer.size_in_bytes);
	auto &aggregatedMessages = devices.at(id).getAggregatedMessages();
	aggregatedMessages.push(forcedStatusBuffer);

	return aggregatedMessages.size();
}

int StatusAggregator::is_device_valid(const struct ::device_identification device) {
	if(is_device_type_supported(device.device_type) == OK &&
	   devices.contains(common_utils::ProtobufUtils::getId(device))) {
		return OK;
	}
	return NOT_OK;
}

int StatusAggregator::get_module_number() { return module_->getModuleNumber(); }

int StatusAggregator::update_command(const struct ::buffer command, const struct ::device_identification device) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}

	std::string id = common_utils::ProtobufUtils::getId(device);
	if(not devices.contains(id)) {
		return DEVICE_NOT_REGISTERED;
	}

	if(module_->commandDataValid(command, device_type) == NOT_OK) {
		log::logWarning("Invalid command data on device: {}", id);
		return COMMAND_INVALID;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	auto &currCommand = devices.at(id).command;
	deallocate(&currCommand);
	currCommand = command;
	return OK;
}

// maybe not registered
int StatusAggregator::get_command(const struct ::buffer status, const struct ::device_identification device,
								  struct ::buffer *command) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Device type {} is not supported", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	std::string id = common_utils::ProtobufUtils::getId(device);
	auto &currStatus = devices[id].getStatus();

	if(status.size_in_bytes == 0 || module_->statusDataValid(status, device_type) == NOT_OK) {
		log::logWarning("Invalid status data on device id: {}", id);
		return STATUS_INVALID;
	}

	struct buffer generatedCommandBuffer {};
	std::lock_guard<std::mutex> lock(mutex_);
	auto &currCommand = devices.at(id).command;
	module_->generateCommand(&generatedCommandBuffer, status, currStatus, currCommand, device_type);
	deallocate(&currCommand);
	currCommand = generatedCommandBuffer;

	int currCommandSize = currCommand.size_in_bytes;
	if(allocate(command, currCommandSize) == NOT_OK) {
		log::logError("Could not allocate memory for command message");
		return NOT_OK;
	}
	std::memcpy(command->data, currCommand.data, currCommandSize);
	command->size_in_bytes = currCommandSize;
	return OK;
}

int StatusAggregator::is_device_type_supported(unsigned int device_type) {
	return module_->isDeviceTypeSupported(device_type);
}

}