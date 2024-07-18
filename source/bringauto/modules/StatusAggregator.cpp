#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/common_utils/MemoryUtils.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>


namespace bringauto::modules {

using log = bringauto::logging::Logger;

int StatusAggregator::clear_device(const structures::DeviceIdentification &key) {
	if(is_device_valid(key) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	if (not devices.contains(key)) {
		return NOT_OK;
	}
	auto &deviceState = devices.at(key);
	auto &aggregatedMessages = deviceState.aggregatedMessages();
	while(not aggregatedMessages.empty()) {
		auto message = aggregatedMessages.front();
		// if(message.data != nullptr) {
		// 	moduleDeallocate(&message);
		// }
		aggregatedMessages.pop();
	}
	// deviceState.deallocateStatus();
	// std::lock_guard<std::mutex> lock(mutex_);
	// deviceState.deallocateCommand();
	return OK;
}

bringauto::modules::Buffer
StatusAggregator::aggregateStatus(structures::StatusAggregatorDeviceState &deviceState, const bringauto::modules::Buffer &status,
								  const unsigned int &device_type) {
	auto &currStatus = deviceState.getStatus();
	bringauto::modules::Buffer aggregatedStatusBuff {};
	module_->aggregateStatus(&aggregatedStatusBuff, currStatus, status, device_type);
	return aggregatedStatusBuff;
}

void StatusAggregator::aggregateSetStatus(structures::StatusAggregatorDeviceState &deviceState, const bringauto::modules::Buffer &status,
										  const unsigned int &device_type) {
	auto aggregatedStatusBuff = aggregateStatus(deviceState, status, device_type);
	deviceState.setStatus(aggregatedStatusBuff);
}

void
StatusAggregator::aggregateSetSendStatus(structures::StatusAggregatorDeviceState &deviceState, const bringauto::modules::Buffer &status,
										 const unsigned int &device_type) {
	auto aggregatedStatusBuff = aggregateStatus(deviceState, status, device_type);
	deviceState.setStatusAndResetTimer(aggregatedStatusBuff);

	auto &currStatus = deviceState.getStatus();
	bringauto::modules::Buffer statusToSendBuff {};
	//moduleAllocate(&statusToSendBuff, currStatus.size_in_bytes);
	//std::memcpy(statusToSendBuff.data, currStatus.data, currStatus.size_in_bytes);
	statusToSendBuff.setStructBuffer(currStatus.getStructBuffer().data, currStatus.getStructBuffer().size_in_bytes);

	auto &aggregatedMessages = deviceState.aggregatedMessages();
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

int StatusAggregator::remove_device(const structures::DeviceIdentification& device) {
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	clear_device(device);

	// WUT, maybe becose there can be multiple context running so by this we ensure no rece condition?
	boost::asio::post(context_->ioContext, [this, device]() { devices.erase(device); });
	return OK;
}

int StatusAggregator::add_status_to_aggregator(const bringauto::modules::Buffer status,
											   const structures::DeviceIdentification& device) {
	const auto &device_type = device.getDeviceType();
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Trying to add status to unsupported device type: {}", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	deviceTimeouts_[device] = 0;
	if(not devices.contains(device)) {
		bringauto::modules::Buffer commandBuffer {};
		module_->generateFirstCommand(&commandBuffer, device_type);
		bringauto::modules::Buffer statusBuffer {};
		// moduleAllocate(&statusBuffer, status.size_in_bytes);
		// std::memcpy(statusBuffer.data, status.data, status.size_in_bytes);
		statusBuffer.setStructBuffer(status.getStructBuffer().data, status.getStructBuffer().size_in_bytes);

		std::function<int(const structures::DeviceIdentification&)> timeouted_force_aggregation = [device, this](
				const structures::DeviceIdentification& deviceId) {
					timeoutedMessageReady_.store(true);
					deviceTimeouts_[device]++;
					return force_aggregation_on_device(deviceId);
		};
		// std::function<void(struct buffer *)> dealloc = [this](
		// 		struct buffer *ptr) { moduleDeallocate(ptr); };
		devices.insert(
				{ device, structures::StatusAggregatorDeviceState(context_, timeouted_force_aggregation, device, commandBuffer, statusBuffer) });

		force_aggregation_on_device(device);
		return 1;
	}

	auto &deviceState = devices.at(device);
	auto &currStatus = deviceState.getStatus();
	auto &aggregatedMessages = deviceState.aggregatedMessages();
	if(module_->sendStatusCondition(currStatus, status, device_type) == OK) {
		aggregateSetSendStatus(deviceState, status, device_type);
	} else {
		aggregateSetStatus(deviceState, status, device_type);
	}

	return aggregatedMessages.size();
}

int StatusAggregator::get_aggregated_status(bringauto::modules::Buffer *generated_status,
											const structures::DeviceIdentification& device) {
	if(is_device_valid(device) == NOT_OK) {
		log::logError("Trying to get aggregated status from unregistered device");
		return DEVICE_NOT_REGISTERED;
	}

	auto &aggregatedMessages = devices.at(device).aggregatedMessages();
	if(aggregatedMessages.empty()) {
		return NO_MESSAGE_AVAILABLE;
	}

	auto &status = aggregatedMessages.front();
	generated_status->setStructBuffer(status.getStructBuffer().data, status.getStructBuffer().size_in_bytes);
	aggregatedMessages.pop();
	return OK;
}

int StatusAggregator::get_unique_devices(bringauto::modules::Buffer *unique_devices_buffer) {
	const auto devicesSize = devices.size();
	if (devicesSize == 0) {
		return 0;
	}

	// if(allocate(unique_devices_buffer, sizeof(struct device_identification) * devicesSize) == NOT_OK) {
	// 	log::logError("Could not allocate buffer in get_unique_devices");
	// 	return NOT_OK;
	// }

	auto *devicesPointer = static_cast<device_identification *>(unique_devices_buffer->getStructBuffer().data);
	int i = 0;
	for(auto const &[key, value]: devices) {
		devicesPointer[i].module = key.getModule();
		devicesPointer[i].device_type = key.getDeviceType();
		common_utils::MemoryUtils::initBuffer(devicesPointer[i].device_role, key.getDeviceRole());
		common_utils::MemoryUtils::initBuffer(devicesPointer[i].device_name, key.getDeviceName());
        devicesPointer[i].priority = key.getPriority();
		i++;
	}

	return devicesSize;
}

int StatusAggregator::force_aggregation_on_device(const structures::DeviceIdentification& device) {
	if(is_device_valid(device) == NOT_OK) {
		log::logError("Trying to force aggregation on unregistered device: {}", device.convertToString());
		return DEVICE_NOT_REGISTERED;
	}

	const auto &statusBuffer = devices.at(device).getStatus();
	bringauto::modules::Buffer forcedStatusBuffer {};
	// if(moduleAllocate(&forcedStatusBuffer, statusBuffer.size_in_bytes) == NOT_OK) {
	// 	log::logError("Could not allocate buffer in force_aggregation_on_device");
	// 	return NOT_OK;
	// }

	// std::memcpy(forcedStatusBuffer.data, statusBuffer.data, statusBuffer.size_in_bytes);
	forcedStatusBuffer.setStructBuffer(statusBuffer.getStructBuffer().data, statusBuffer.getStructBuffer().size_in_bytes);
	auto &aggregatedMessages = devices.at(device).aggregatedMessages();
	aggregatedMessages.push(forcedStatusBuffer);

	return aggregatedMessages.size();
}

int StatusAggregator::is_device_valid(const structures::DeviceIdentification& device) {
	if(is_device_type_supported(device.getDeviceType()) == OK &&
	   devices.contains(device)) {
		return OK;
	}
	return NOT_OK;
}

int StatusAggregator::get_module_number() { return module_->getModuleNumber(); }

int StatusAggregator::update_command(const bringauto::modules::Buffer command, const structures::DeviceIdentification& device) {
	const auto &device_type = device.getDeviceType();
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Device type {} is not supported", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	if(not devices.contains(device)) {
		log::logWarning("Received command for not registered device: {}", device.convertToString());
		return DEVICE_NOT_REGISTERED;
	}

	if(module_->commandDataValid(command, device_type) == NOT_OK) {
		log::logWarning("Invalid command data on device: {}", device.convertToString());
		return COMMAND_INVALID;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	devices.at(device).setCommand(command);
	return OK;
}

int StatusAggregator::get_command(const bringauto::modules::Buffer status, const structures::DeviceIdentification& device,
								  bringauto::modules::Buffer *command) {
	const auto &device_type = device.getDeviceType();
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Device type {} is not supported", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	if(status.getStructBuffer().size_in_bytes == 0 || module_->statusDataValid(status, device_type) == NOT_OK) {
		log::logWarning("Invalid status data on device id: {}", device.convertToString());
		return STATUS_INVALID;
	}

	if(is_device_valid(device) == NOT_OK) {
		module_->generateFirstCommand(command, device_type);
		return OK;
	}

	auto &deviceState = devices.at(device);
	bringauto::modules::Buffer generatedCommandBuffer {};
	std::lock_guard<std::mutex> lock(mutex_);
	auto &currCommand = devices.at(device).getCommand();
	module_->generateCommand(&generatedCommandBuffer, status, deviceState.getStatus(), deviceState.getCommand(),
							 device_type);
	deviceState.setCommand(generatedCommandBuffer);

	// auto currCommandSize = currCommand.size_in_bytes;
	// if(moduleAllocate(command, currCommandSize) == NOT_OK) {
	// 	log::logError("Could not allocate memory for command message");
	// 	return NOT_OK;
	// }
	// std::memcpy(command->data, currCommand.data, currCommandSize);
	command->setStructBuffer(currCommand.getStructBuffer().data, currCommand.getStructBuffer().size_in_bytes);
	// command->size_in_bytes = currCommandSize;
	return OK;
}

int StatusAggregator::is_device_type_supported(unsigned int device_type) {
	return module_->isDeviceTypeSupported(device_type);
}

// int StatusAggregator::moduleAllocate(struct buffer *buffer, size_t size_in_bytes){
// 	return module_->allocate(buffer, size_in_bytes);
// }

// void StatusAggregator::moduleDeallocate(struct buffer *buffer){
// 	module_->deallocate(buffer);
// }

void StatusAggregator::unsetTimeoutedMessageReady(){
	timeoutedMessageReady_.store(false);
}

bool StatusAggregator::getTimeoutedMessageReady() const {
	return timeoutedMessageReady_.load();
}

int StatusAggregator::getDeviceTimeoutCount(const structures::DeviceIdentification &key) {
	return deviceTimeouts_[key];
}

}