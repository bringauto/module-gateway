#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/settings/LoggerId.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>

#include <fleet_protocol/module_gateway/error_codes.h>


namespace bringauto::modules {

using log = settings::Logger;

int StatusAggregator::clear_device(const structures::DeviceIdentification &device) {
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	auto &deviceState = devices.at(device);
	auto &aggregatedMessages = deviceState.aggregatedMessages();
	while(not aggregatedMessages.empty()) {
		aggregatedMessages.pop();
	}
	return OK;
}

Buffer
StatusAggregator::aggregateStatus(const structures::StatusAggregatorDeviceState &deviceState, const Buffer &status,
								  const unsigned int &device_type) const {
	auto &currStatus = deviceState.getStatus();
	Buffer aggregatedStatusBuff {};
	if (module_->aggregateStatus(aggregatedStatusBuff, currStatus, status, device_type) != OK) {
		log::logWarning("Error occurred while aggregating status, returning current status buffer");
		return currStatus;
	}
	return aggregatedStatusBuff;
}

void StatusAggregator::aggregateSetStatus(structures::StatusAggregatorDeviceState &deviceState, const Buffer &status,
										  const unsigned int &device_type) const {
	const auto aggregatedStatusBuff = aggregateStatus(deviceState, status, device_type);
	deviceState.setStatus(aggregatedStatusBuff);
}

void
StatusAggregator::aggregateSetSendStatus(structures::StatusAggregatorDeviceState &deviceState, const Buffer &status,
										 const unsigned int &device_type) const {
	const auto aggregatedStatusBuff = aggregateStatus(deviceState, status, device_type);
	deviceState.setStatusAndResetTimer(aggregatedStatusBuff);

	auto &aggregatedMessages = deviceState.aggregatedMessages();
	aggregatedMessages.push(aggregatedStatusBuff);
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

int StatusAggregator::add_status_to_aggregator(const Buffer& status,
											   const structures::DeviceIdentification& device) {
	const auto &device_type = device.getDeviceType();
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Trying to add status to unsupported device type: {}", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	deviceTimeouts_[device] = 0;
	if(not devices.contains(device)) {
		Buffer commandBuffer {};
		if (module_->generateFirstCommand(commandBuffer, device_type) != OK) {
			log::logError("Failed to generate first command for device: {}", device.convertToString());
			return COMMAND_INVALID;
		}

		const std::function<int(const structures::DeviceIdentification&)> timeouted_force_aggregation = [device, this](
				const structures::DeviceIdentification& deviceId) {
					timeoutedMessageReady_.store(true);
					deviceTimeouts_[device]++;
					return force_aggregation_on_device(deviceId);
		};
		devices.emplace(
				device, structures::StatusAggregatorDeviceState(context_, timeouted_force_aggregation, device, commandBuffer, status));

		force_aggregation_on_device(device);
		return 1;
	}

	auto &deviceState = devices.at(device);
	auto &currStatus = deviceState.getStatus();
	const auto &aggregatedMessages = deviceState.aggregatedMessages();
	if(module_->sendStatusCondition(currStatus, status, device_type) == OK) {
		aggregateSetSendStatus(deviceState, status, device_type);
	} else {
		aggregateSetStatus(deviceState, status, device_type);
	}

	return aggregatedMessages.size();
}

int StatusAggregator::get_aggregated_status(Buffer &generated_status,
											const structures::DeviceIdentification& device) {
	if(is_device_valid(device) == NOT_OK) {
		log::logError("Trying to get aggregated status from unregistered device");
		return DEVICE_NOT_REGISTERED;
	}

	auto &aggregatedMessages = devices.at(device).aggregatedMessages();
	if(aggregatedMessages.empty()) {
		return NO_MESSAGE_AVAILABLE;
	}

	generated_status = aggregatedMessages.front();
	aggregatedMessages.pop();
	return OK;
}

int StatusAggregator::get_unique_devices(std::list<structures::DeviceIdentification> &unique_devices_list) {
	const auto devicesSize = devices.size();
	if (devicesSize == 0) {
		return 0;
	}

	for(auto& [deviceId, _]: devices) {
		unique_devices_list.push_back(deviceId);
	}

	return devicesSize;
}

int StatusAggregator::force_aggregation_on_device(const structures::DeviceIdentification& device) {
	if(is_device_valid(device) == NOT_OK) {
		log::logError("Trying to force aggregation on unregistered device: {}", device.convertToString());
		return DEVICE_NOT_REGISTERED;
	}

	const auto &statusBuffer = devices.at(device).getStatus();
	auto &aggregatedMessages = devices.at(device).aggregatedMessages();
	aggregatedMessages.push(statusBuffer);
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

int StatusAggregator::update_command(const Buffer& command, const structures::DeviceIdentification& device) {
	const auto &device_type = device.getDeviceType();
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Device type {} is not supported", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	if(not devices.contains(device)) {
		log::logWarning("Received command for not registered device: {}", device.convertToString());
		return DEVICE_NOT_REGISTERED;
	}

	if(!command.isAllocated() || module_->commandDataValid(command, device_type) == NOT_OK) {
		log::logWarning("Invalid command data on device: {}", device.convertToString());
		return COMMAND_INVALID;
	}

	if (devices.at(device).addExternalCommand(command) == NOT_OK) {
		log::logError("External command queue is full for device: {} deleting oldest command", device.convertToString());
	}
	return OK;
}

int StatusAggregator::get_command(const Buffer& status, const structures::DeviceIdentification& device,
								  Buffer& command) {
	const auto &device_type = device.getDeviceType();
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Device type {} is not supported", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	auto &deviceState = devices.at(device);
	if (module_->generateCommand(command, status, deviceState.getStatus(), deviceState.getCommand(), device_type) != OK) {
		log::logError("Error occurred while generating command for device: {}", device.convertToString());
		return COMMAND_INVALID;
	}

	deviceState.setDefaultCommand(command);
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

int StatusAggregator::getDeviceTimeoutCount(const structures::DeviceIdentification &device) const {
	if(const auto it = deviceTimeouts_.find(device); it != deviceTimeouts_.end()) {
		return it->second;
	}
	return 0;
}

}
