#include <bringauto/external_client/ErrorAggregator.hpp>

#include <module_manager.h>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <mg_error_codes.h>



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
ErrorAggregator::add_status_to_error_aggregator(const struct buffer status, const struct device_identification device) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}
	std::string id = common_utils::ProtobufUtils::getId(device);

	if(status.size_in_bytes == 0) {
		log::logWarning("Invalid status data for device: {}", id);
		return NOT_OK;
	}

	if(not devices_.contains(id)) {
		devices_.insert({ id, {}});
	}

	auto &lastStatus = devices_[id].lastStatus;
	if(lastStatus.data != nullptr && lastStatus.size_in_bytes > 0) { // status.size_in_bytes > lastStatus.size_in_bytes
		module_->deallocate(&lastStatus);
	}
	if(module_->allocate(&lastStatus, status.size_in_bytes) == NOT_OK) {
		log::logError("Could not allocate memory for status buffer in function add_status_to_error_aggregator");
		return NOT_OK;
	}
	std::memcpy(lastStatus.data, status.data, status.size_in_bytes);

	struct buffer errorMessageBuffer {};
	auto &currentError = devices_[id].errorMessage;

	auto retCode = module_->aggregateError(&errorMessageBuffer, currentError, status, device_type);
	if(retCode != OK) {
		log::logWarning("Error occurred in Error aggregator for device: {}", id);
		return NOT_OK;
	}
	if(currentError.data != nullptr) {
		module_->deallocate(&currentError);
	}
	currentError = errorMessageBuffer;
	return OK;
}

int ErrorAggregator::get_last_status(struct buffer *status, const struct device_identification device) {
	std::string id = common_utils::ProtobufUtils::getId(device);
	if(not devices_.contains(id)) {
		return DEVICE_NOT_REGISTERED;
	}

	auto &lastStatus = devices_[id].lastStatus;

	if(lastStatus.data == nullptr || lastStatus.size_in_bytes == 0) {
		return NO_MESSAGE_AVAILABLE;
	}
	status->data = lastStatus.data;
	status->size_in_bytes = lastStatus.size_in_bytes;
	return OK;
}

int ErrorAggregator::get_error(struct buffer *error, const struct device_identification device) {
	std::string id = common_utils::ProtobufUtils::getId(device);
	if(not devices_.contains(id)) {
		return DEVICE_NOT_REGISTERED;
	}

	auto &currentError = devices_[id].errorMessage;

	if(currentError.data == nullptr || currentError.size_in_bytes == 0) {
		return NO_MESSAGE_AVAILABLE;
	}
	error->data = currentError.data;
	error->size_in_bytes = currentError.size_in_bytes;
	return OK;
}

int ErrorAggregator::clear_error_aggregator() {
	for(auto &[key, device]: devices_) {
		if(device.lastStatus.data != nullptr && device.lastStatus.size_in_bytes > 0) {
			module_->deallocate(&device.lastStatus);
		}
		if(device.errorMessage.data != nullptr && device.errorMessage.size_in_bytes > 0) {
			module_->deallocate(&device.errorMessage);
		}
	}
	return OK;
}

int ErrorAggregator::get_module_number() const {
	return module_->getModuleNumber();
}

int ErrorAggregator::is_device_type_supported(unsigned int device_type) {
	return module_->isDeviceTypeSupported(device_type);
}

}