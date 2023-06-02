#include <bringauto/modules/ErrorAggregator.hpp>

#include <module_manager.h>
#include <bringauto/logging/Logger.hpp>
#include <mg_error_codes.h>

#include <dlfcn.h>
#include <sstream>



namespace bringauto::modules {

template <typename T>
struct FunctionTypeDeducer;

template<typename R, typename ...Args>
struct FunctionTypeDeducer<std::function<R(Args...)>>{
	using fncptr = R (*)(Args...);
};

using log = bringauto::logging::Logger;

std::string ErrorAggregator::getId(const ::device_identification &device) {
	std::stringstream ss;
	ss << device.module << "/" << device.device_type << "/" << device.device_role << "/" << device.device_name;
	return ss.str();
}

int bringauto::modules::ErrorAggregator::init_error_aggregator(std::filesystem::path path) {
	module = dlopen(path.c_str(), RTLD_LAZY | RTLD_DEEPBIND);
	if(module == nullptr) {
		return NOT_OK;
	}
	isDeviceTypeSupported = reinterpret_cast<FunctionTypeDeducer<decltype(isDeviceTypeSupported)>::fncptr>(dlsym(module, "is_device_type_supported"));
	getModuleNumber = reinterpret_cast<FunctionTypeDeducer<decltype(getModuleNumber)>::fncptr>(dlsym(module, "get_module_number"));
	aggregateError = reinterpret_cast<FunctionTypeDeducer<decltype(aggregateError)>::fncptr>(dlsym(module, "aggregate_error"));
	return OK;
}

int ErrorAggregator::destroy_error_aggregator() {
	if(module == nullptr) {
		return OK;
	}
	dlclose(module);
	module = nullptr;
	clear_error_aggregator();
	return OK;
}

int
ErrorAggregator::add_status_to_error_aggregator(const struct buffer status, const struct device_identification device) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}
	std::string id = getId(device);

	if(status.size_in_bytes == 0) {
		log::logWarning("Invalid status data for device: {}", id);
		return NOT_OK;
	}

	if(not devices.contains(id)) {
		devices.insert({ id, { } });
	}

	auto &lastStatus = devices[id].lastStatus;
	if (status.size_in_bytes > lastStatus.size_in_bytes) {
		deallocate(&lastStatus);
		allocate(&lastStatus, status.size_in_bytes);
	}
	lastStatus.data = status.data;
	lastStatus.size_in_bytes = status.size_in_bytes;

	struct buffer errorMessageBuffer {};
	auto &currentError = devices[id].errorMessage;

	auto retCode = aggregateError(&errorMessageBuffer, currentError, status, device_type);
	if (retCode == WRONG_FORMAT) {
		log::logWarning("Wrong status format in Error aggregator for device: {}", id);
		return NOT_OK;
	} else if (retCode != OK) {
		log::logWarning("Error occurred in Error aggregator for device: {}", id);
		return NOT_OK;
	}
	deallocate(&currentError);
	currentError = errorMessageBuffer;
	return OK;
}

int ErrorAggregator::get_last_status(struct buffer *status, const struct device_identification device) {
	std::string id = getId(device);
	if (not devices.contains(id)) {
		return DEVICE_NOT_REGISTERED;
	}

	auto &lastStatus = devices[id].lastStatus;

	if (lastStatus.data == nullptr || lastStatus.size_in_bytes == 0) {
		return NO_MESSAGE_AVAILABLE;
	}
	allocate(status, lastStatus.size_in_bytes);
	status->data = lastStatus.data;
	status->size_in_bytes = lastStatus.size_in_bytes;
	return OK;
	}

int ErrorAggregator::get_error(struct buffer *error, const struct device_identification device) {
	std::string id = getId(device);
	if (not devices.contains(id)) {
		return DEVICE_NOT_REGISTERED;
	}

	auto &currentError = devices[id].errorMessage;

	if (currentError.data == nullptr || currentError.size_in_bytes == 0) {
		return NO_MESSAGE_AVAILABLE;
	}
	allocate(error, currentError.size_in_bytes);
	error->data = currentError.data;
	error->size_in_bytes = currentError.size_in_bytes;
	return OK;
}

int ErrorAggregator::clear_error_aggregator() {
	for (auto& [key, device] : devices) {
		deallocate(&device.lastStatus);
		deallocate(&device.errorMessage);
	}
	devices.clear();
	return OK;
}

int ErrorAggregator::get_module_number() {
	return getModuleNumber();
}

int ErrorAggregator::is_device_type_supported(unsigned int device_type) {
	return isDeviceTypeSupported(device_type);
}

}