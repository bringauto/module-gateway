#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/logging/Logger.hpp>
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
		aggregatedMessages.pop();
	}
	return OK;
}

bringauto::modules::Buffer
StatusAggregator::aggregateStatus(structures::StatusAggregatorDeviceState &deviceState, const bringauto::modules::Buffer &status,
								  const unsigned int &device_type) {
	auto &currStatus = deviceState.getStatus();
	bringauto::modules::Buffer aggregatedStatusBuff = module_->constructBuffer();
	module_->aggregateStatus(aggregatedStatusBuff, currStatus, status, device_type);
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
	bringauto::modules::Buffer statusToSendBuff = module_->constructBuffer();
	statusToSendBuff = currStatus;

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

int StatusAggregator::add_status_to_aggregator(const bringauto::modules::Buffer& status,
											   const structures::DeviceIdentification& device) {
	const auto &device_type = device.getDeviceType();
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Trying to add status to unsupported device type: {}", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	deviceTimeouts_[device] = 0;
	if(not devices.contains(device)) {
		bringauto::modules::Buffer commandBuffer = module_->constructBuffer();
		module_->generateFirstCommand(commandBuffer, device_type);
		bringauto::modules::Buffer statusBuffer = module_->constructBuffer();
		statusBuffer = status;

		std::function<int(const structures::DeviceIdentification&)> timeouted_force_aggregation = [device, this](
				const structures::DeviceIdentification& deviceId) {
					timeoutedMessageReady_.store(true);
					deviceTimeouts_[device]++;
					return force_aggregation_on_device(deviceId);
		};
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

int StatusAggregator::get_aggregated_status(bringauto::modules::Buffer &generated_status,
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
	generated_status = status;
	aggregatedMessages.pop();
	return OK;
}

int StatusAggregator::get_unique_devices(std::list<structures::DeviceIdentification> &unique_devices_list) {
	const auto devicesSize = devices.size();
	if (devicesSize == 0) {
		return 0;
	}

	for(auto &device: devices) {
		unique_devices_list.push_back(device.first);
	}

	return devicesSize;
}

int StatusAggregator::force_aggregation_on_device(const structures::DeviceIdentification& device) {
	if(is_device_valid(device) == NOT_OK) {
		log::logError("Trying to force aggregation on unregistered device: {}", device.convertToString());
		return DEVICE_NOT_REGISTERED;
	}

	const auto &statusBuffer = devices.at(device).getStatus();
	bringauto::modules::Buffer forcedStatusBuffer = module_->constructBuffer();
	forcedStatusBuffer = statusBuffer;
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

int StatusAggregator::update_command(const bringauto::modules::Buffer& command, const structures::DeviceIdentification& device) {
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

	if (devices.at(device).addExternalCommand(command) == NOT_OK) {
		log::logError("External command queue is full for device: {} deleting oldest command", device.convertToString());
	}
	return OK;
}

int StatusAggregator::get_command(const bringauto::modules::Buffer& status, const structures::DeviceIdentification& device,
								  bringauto::modules::Buffer& command) {
	const auto &device_type = device.getDeviceType();
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Device type {} is not supported", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	if(status.getStructBuffer().size_in_bytes == 0 || module_->statusDataValid(status, device_type) == NOT_OK) {
		log::logWarning("Invalid status data on device id: {}", device.convertToString());
		return STATUS_INVALID;
	}

	auto &deviceState = devices.at(device);
	bringauto::modules::Buffer generatedCommandBuffer = module_->constructBuffer();
	auto &currCommand = deviceState.getCommand();
	module_->generateCommand(generatedCommandBuffer, status, deviceState.getStatus(), currCommand,
							 device_type);
	deviceState.setCommand(generatedCommandBuffer);
	command = generatedCommandBuffer;
	return OK;
}

int StatusAggregator::is_device_type_supported(unsigned int device_type) {
	return module_->isDeviceTypeSupported(device_type);
}

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