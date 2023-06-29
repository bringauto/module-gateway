#include <dlfcn.h>
#include <sstream>

#include <bringauto/logging/Logger.hpp>
#include <bringauto/modules/StatusAggregator.hpp>



namespace bringauto::modules {

using log = bringauto::logging::Logger;

std::string StatusAggregator::getId(const ::device_identification &device) {
	std::stringstream ss;
	ss << device.module << "/" << device.device_type << "/" << device.device_role << "/" << device.device_name; // TODO we need to be able to get priority, maybe separate variable
	return ss.str();
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
		auto &aggregatedMessages = device.aggregatedMessages;
		while(not aggregatedMessages.empty()) {
			auto message = aggregatedMessages.front();
			if(message.data != nullptr) {
				deallocate(&message);
			}
			aggregatedMessages.pop();
		}
		deallocate(&device.status);
		std::lock_guard <std::mutex> lock(mutex_);
		deallocate(&device.command);
	}
	devices.clear();
	return OK;
}

int StatusAggregator::clear_device(const struct ::device_identification device) {
	std::string id = getId(device);
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	auto &aggregatedMessages = devices[id].aggregatedMessages;
	while(not aggregatedMessages.empty()) {
		aggregatedMessages.pop();
	}
	return OK;
}

int StatusAggregator::remove_device(const struct ::device_identification device) {
	std::string id = getId(device);
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	devices.erase(id);
	return OK;
}

int StatusAggregator::add_status_to_aggregator(const struct ::buffer status,
											   const struct ::device_identification device) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}

	std::string id = getId(device);

	if(status.size_in_bytes == 0 || module_->statusDataValid(status, device_type) == NOT_OK) {
		log::logWarning("Invalid status data on device: {}", id);
		return NOT_OK;
	}

	if(not devices.contains(id)) {
		struct buffer commandBuffer {};
		module_->generateFirstCommand(&commandBuffer, device_type);
		struct buffer statusBuffer {};
		allocate(&statusBuffer, status.size_in_bytes);
		std::memcpy(statusBuffer.data, status.data, status.size_in_bytes );
		devices.insert({ id, { commandBuffer, statusBuffer }});
		return 0;
	}

	auto &currStatus = devices[id].status;
	auto &aggregatedMessages = devices[id].aggregatedMessages;
	if(module_->sendStatusCondition(currStatus, status, device_type) == OK) {
		struct buffer aggregatedStatusBuff {};
		module_->aggregateStatus(&aggregatedStatusBuff, currStatus, status, device_type);

		deallocate(&currStatus);
		currStatus = aggregatedStatusBuff;
	} else {
		aggregatedMessages.push(currStatus);
		allocate(&currStatus, status.size_in_bytes);
		std::memcpy(currStatus.data, status.data, status.size_in_bytes);
	}

	return aggregatedMessages.size();
}

int StatusAggregator::get_aggregated_status(struct ::buffer *generated_status,
											const struct ::device_identification device) {
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}

	std::string id = getId(device);
	auto &aggregatedMessages = devices[id].aggregatedMessages;
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
	int ret = allocate(unique_devices_buffer, str.size());
	if(ret == NOT_OK) {
		log::logError("Could not allocate buffer in get_unique_devices");
		return NOT_OK;
	}
	std::memcpy(unique_devices_buffer->data, str.c_str(), str.size());
	//strncpy(static_cast<char *>(unique_devices_buffer->data), str.c_str(), str.size());
	return devices.size();
}

int StatusAggregator::force_aggregation_on_device(const struct ::device_identification device) {
	std::string id = getId(device);
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}

	auto &aggregatedMessages = devices[id].aggregatedMessages;
    const auto &statusBuffer = devices[id].status;
    struct buffer forcedStatusBuffer {};
    allocate(&forcedStatusBuffer, statusBuffer.size_in_bytes);
	std::memcpy(forcedStatusBuffer.data, statusBuffer.data, statusBuffer.size_in_bytes);
	aggregatedMessages.push(forcedStatusBuffer);
	return aggregatedMessages.size();
}

int StatusAggregator::is_device_valid(const struct ::device_identification device) {
	if(is_device_type_supported(device.device_type) == OK && devices.contains(getId(device))) {
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

	std::string id = getId(device);
	if(not devices.contains(id)) {
		return DEVICE_NOT_REGISTERED;
	}

	if(module_->commandDataValid(command, device_type) == NOT_OK) {
		log::logWarning("Invalid command data on device: {}", id);
		return COMMAND_INVALID;
	}

	std::lock_guard <std::mutex> lock(mutex_);
	auto &currCommand = devices[id].command;
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

	std::string id = getId(device);
	auto &currStatus = devices[id].status;

	if(status.size_in_bytes == 0 || module_->statusDataValid(status, device_type) == NOT_OK) {
		log::logWarning("Invalid status data on device id: {}", id);
		return STATUS_INVALID;
	}

	struct buffer generatedCommandBuffer {};
	std::lock_guard <std::mutex> lock(mutex_);
	auto &currCommand = devices[id].command;
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