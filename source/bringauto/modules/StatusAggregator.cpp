#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/common_utils/MemoryUtils.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/common_utils/StringUtils.hpp>



namespace bringauto::modules {

using log = bringauto::logging::Logger;

int StatusAggregator::clear_device(const std::string &key) {
	if (not devices.contains(key)) {
		return NOT_OK;
	}
	auto &deviceState = devices.at(key);
	auto &aggregatedMessages = deviceState.getAggregatedMessages();
	while(not aggregatedMessages.empty()) {
		auto message = aggregatedMessages.front();
		if(message.data != nullptr) {
			moduleDeallocate(&message);
		}
		aggregatedMessages.pop();
	}
	deviceState.deallocateStatus();
	std::lock_guard<std::mutex> lock(mutex_);
	deviceState.deallocateCommand();
	return OK;
}

struct buffer
StatusAggregator::aggregateStatus(structures::StatusAggregatorDeviceState &deviceState, const buffer &status,
								  const unsigned int &device_type) {
	auto &currStatus = deviceState.getStatus();
	struct buffer aggregatedStatusBuff {};
	module_->aggregateStatus(&aggregatedStatusBuff, currStatus, status, device_type);
	return aggregatedStatusBuff;
}

void StatusAggregator::aggregateSetStatus(structures::StatusAggregatorDeviceState &deviceState, const buffer &status,
										  const unsigned int &device_type) {
	auto aggregatedStatusBuff = aggregateStatus(deviceState, status, device_type);
	deviceState.setStatus(aggregatedStatusBuff);
}

void
StatusAggregator::aggregateSetSendStatus(structures::StatusAggregatorDeviceState &deviceState, const buffer &status,
										 const unsigned int &device_type) {
	auto aggregatedStatusBuff = aggregateStatus(deviceState, status, device_type);
	deviceState.setStatusAndResetTimer(aggregatedStatusBuff);

	auto &currStatus = deviceState.getStatus();
	struct buffer statusToSendBuff {};
	moduleAllocate(&statusToSendBuff, currStatus.size_in_bytes);
	std::memcpy(statusToSendBuff.data, currStatus.data, currStatus.size_in_bytes);

	auto &aggregatedMessages = deviceState.getAggregatedMessages();
	aggregatedMessages.push(statusToSendBuff);
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
	return clear_device(id);
}

int StatusAggregator::remove_device(const struct ::device_identification device) {
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	std::string id = common_utils::ProtobufUtils::getId(device);
	clear_device(device);
	boost::asio::post(context_->ioContext, [this, id]() { devices.erase(id); });
	return OK;
}

int StatusAggregator::add_status_to_aggregator(const struct ::buffer status,
											   const struct ::device_identification device) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Trying to add status to unsupported device type: {}", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	std::string id = common_utils::ProtobufUtils::getId(device);
	deviceTimeouts_[id] = 0;

	if(not devices.contains(id)) {
		struct buffer commandBuffer {};
		module_->generateFirstCommand(&commandBuffer, device_type);
		struct buffer statusBuffer {};
		moduleAllocate(&statusBuffer, status.size_in_bytes);
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

		std::function<int(struct ::device_identification)> timeouted_force_aggregation = [id, this](
				struct ::device_identification deviceId) {
					timeoutedMessageReady_.store(true);
					deviceTimeouts_[id]++;
					return force_aggregation_on_device(deviceId);
		};
		std::function<void(struct buffer *)> dealloc = [&](
				struct buffer *device) { moduleDeallocate(device); };
		devices.insert(
				{ id, structures::StatusAggregatorDeviceState(context_, timeouted_force_aggregation, deviceId, commandBuffer, statusBuffer, dealloc) });

		force_aggregation_on_device(device);
		return 1;
	}

	auto &deviceState = devices.at(id);
	auto &currStatus = deviceState.getStatus();
	auto &aggregatedMessages = deviceState.getAggregatedMessages();
	if(module_->sendStatusCondition(currStatus, status, device_type) == OK) {
		aggregateSetSendStatus(deviceState, status, device_type);
	} else {
		aggregateSetStatus(deviceState, status, device_type);
	}

	return aggregatedMessages.size();
}

int StatusAggregator::get_aggregated_status(struct ::buffer *generated_status,
											const struct ::device_identification device) {
	if(is_device_valid(device) == NOT_OK) {
		log::logError("Trying to get aggregated status from unregistered device");
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
	const auto devicesSize = devices.size();
	if (devicesSize == 0) {
		return 0;
	}

	if(allocate(unique_devices_buffer, sizeof(struct device_identification) * devicesSize) == NOT_OK) {
		log::logError("Could not allocate buffer in get_unique_devices");
		return NOT_OK;
	}

	device_identification *devicesPointer = static_cast<device_identification *>(unique_devices_buffer->data);
	int i = 0;
	for(auto const &[key, value]: devices) {
		auto tokenVec = common_utils::StringUtils::splitString(key, '/');
		devicesPointer[i].module = std::stoi(tokenVec[0]);
		devicesPointer[i].device_type = std::stoi(tokenVec[1]);
		common_utils::MemoryUtils::initBuffer(devicesPointer[i].device_role, tokenVec[2]);
		common_utils::MemoryUtils::initBuffer(devicesPointer[i].device_name, tokenVec[3]);
		i++;
	}

	return devicesSize;
}

int StatusAggregator::force_aggregation_on_device(const struct ::device_identification device) {
	std::string id = common_utils::ProtobufUtils::getId(device);
	if(is_device_valid(device) == NOT_OK) {
		log::logError("Trying to force aggregation on unregistered device: {}", id);
		return DEVICE_NOT_REGISTERED;
	}

	const auto &statusBuffer = devices.at(id).getStatus();
	struct buffer forcedStatusBuffer {};
	if(moduleAllocate(&forcedStatusBuffer, statusBuffer.size_in_bytes) == NOT_OK) {
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
		log::logError("Device type {} is not supported", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	std::string id = common_utils::ProtobufUtils::getId(device);
	if(not devices.contains(id)) {
		log::logWarning("Received command for not registered device: {}", id);
		return DEVICE_NOT_REGISTERED;
	}

	if(module_->commandDataValid(command, device_type) == NOT_OK) {
		log::logWarning("Invalid command data on device: {}", id);
		return COMMAND_INVALID;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	devices.at(id).setCommand(command);
	return OK;
}

int StatusAggregator::get_command(const struct ::buffer status, const struct ::device_identification device,
								  struct ::buffer *command) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Device type {} is not supported", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	std::string id = common_utils::ProtobufUtils::getId(device);

	if(status.size_in_bytes == 0 || module_->statusDataValid(status, device_type) == NOT_OK) {
		log::logWarning("Invalid status data on device id: {}", id);
		return STATUS_INVALID;
	}

	if(is_device_valid(device) == NOT_OK) {
		module_->generateFirstCommand(command, device_type);
		return OK;
	}

	auto &deviceState = devices.at(id);
	struct buffer generatedCommandBuffer {};
	std::lock_guard<std::mutex> lock(mutex_);
	auto &currCommand = devices.at(id).getCommand();
	module_->generateCommand(&generatedCommandBuffer, status, deviceState.getStatus(), deviceState.getCommand(),
							 device_type);
	deviceState.setCommand(generatedCommandBuffer);

	int currCommandSize = currCommand.size_in_bytes;
	if(moduleAllocate(command, currCommandSize) == NOT_OK) {
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

int StatusAggregator::moduleAllocate(struct buffer *buffer, size_t size_in_bytes){
	return module_->allocate(buffer, size_in_bytes);
}

void StatusAggregator::moduleDeallocate(struct buffer *buffer){
	module_->deallocate(buffer);
}

void StatusAggregator::unsetTimeoutedMessageReady(){
	timeoutedMessageReady_.store(false);
}

bool StatusAggregator::getTimeoutedMessageReady() const {
	return timeoutedMessageReady_.load();
}

int StatusAggregator::getDeviceTimeoutCount(const std::string &key) {
	return deviceTimeouts_[key];
}

}