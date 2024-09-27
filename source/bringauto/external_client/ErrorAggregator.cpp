#include <bringauto/external_client/ErrorAggregator.hpp>

#include <fleet_protocol/module_maintainer/module_gateway/module_manager.h>
#include <fleet_protocol/module_gateway/error_codes.h>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>



namespace bringauto::external_client {

using log = bringauto::logging::Logger;

int ErrorAggregator::init_error_aggregator(const std::shared_ptr<modules::ModuleManagerLibraryHandler> &library) {
	module_ = library;
	return OK;
}

int ErrorAggregator::destroy_error_aggregator() {
	clear_error_aggregator();
	return OK;
}

int
ErrorAggregator::add_status_to_error_aggregator(const modules::Buffer& status, const structures::DeviceIdentification& device) {
	const auto &device_type = device.getDeviceType();
	if(is_device_type_supported(device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}

	if(not devices_.contains(device)) {
		devices_.insert({ device, {}});
	}

	auto &lastStatus = devices_[device].lastStatus;
	lastStatus = status;

	modules::Buffer errorMessageBuffer {};
	auto &currentError = devices_[device].errorMessage;

	if(module_->aggregateError(errorMessageBuffer, currentError, status, device_type) != OK) {
		log::logWarning("Error occurred in Error aggregator for device: {}", device.convertToString());
		return NOT_OK;
	}
	currentError = errorMessageBuffer;
	return OK;
}

int ErrorAggregator::get_last_status(modules::Buffer &status, const structures::DeviceIdentification& device) {
	if(not devices_.contains(device)) {
		return DEVICE_NOT_REGISTERED;
	}

	status = devices_[device].lastStatus;
	return OK;
}

int ErrorAggregator::get_error(modules::Buffer &error, const structures::DeviceIdentification& device) {
	if(not devices_.contains(device)) {
		return DEVICE_NOT_REGISTERED;
	}

	auto &currentError = devices_[device].errorMessage;

	if(!currentError.isAllocated()) {
		return NO_MESSAGE_AVAILABLE;
	}
	error = currentError;
	return OK;
}

int ErrorAggregator::clear_error_aggregator() {
	return OK;
}

int ErrorAggregator::get_module_number() const {
	return module_->getModuleNumber();
}

int ErrorAggregator::is_device_type_supported(unsigned int device_type) {
	return module_->isDeviceTypeSupported(device_type);
}

}
