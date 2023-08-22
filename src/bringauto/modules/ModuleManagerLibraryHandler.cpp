#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <bringauto/logging/Logger.hpp>
#include <general_error_codes.h>

#include <dlfcn.h>



namespace bringauto::modules {

template <typename T>
struct FunctionTypeDeducer;

template <typename R, typename ...Args>
struct FunctionTypeDeducer<std::function<R(Args...)>> {
	using fncptr = R (*)(Args...);
};

using log = bringauto::logging::Logger;

int ModuleManagerLibraryHandler::loadLibrary(const std::filesystem::path &path) {
	try {
		module_ = dlopen(path.c_str(), RTLD_LAZY);
		if(module_ == nullptr) {
			return NOT_OK;
		}
		isDeviceTypeSupported_ = reinterpret_cast<FunctionTypeDeducer<decltype(isDeviceTypeSupported_)>::fncptr>(dlsym(
				module_, "is_device_type_supported"));
		getModuleNumber_ = reinterpret_cast<FunctionTypeDeducer<decltype(getModuleNumber_)>::fncptr>(dlsym(module_,
																										   "get_module_number"));
		generateFirstCommand_ = reinterpret_cast<FunctionTypeDeducer<decltype(generateFirstCommand_)>::fncptr>(dlsym(
				module_, "generate_first_command"));
		statusDataValid_ = reinterpret_cast<FunctionTypeDeducer<decltype(statusDataValid_)>::fncptr>(dlsym(module_,
																										   "status_data_valid"));
		commandDataValid_ = reinterpret_cast<FunctionTypeDeducer<decltype(commandDataValid_)>::fncptr>(dlsym(module_,
																											 "command_data_valid"));
		sendStatusCondition_ = reinterpret_cast<FunctionTypeDeducer<decltype(sendStatusCondition_)>::fncptr>(dlsym(
				module_, "send_status_condition"));
		aggregateStatus_ = reinterpret_cast<FunctionTypeDeducer<decltype(aggregateStatus_)>::fncptr>(dlsym(module_,
																										   "aggregate_status"));
		generateCommand_ = reinterpret_cast<FunctionTypeDeducer<decltype(generateCommand_)>::fncptr>(dlsym(module_,
																										   "generate_command"));
		aggregateError_ = reinterpret_cast<FunctionTypeDeducer<decltype(aggregateError_)>::fncptr>(dlsym(module_,
																										 "aggregate_error"));
		return OK;
	} catch(const std::exception &e) {
		log::logError("Error occurred during loading library: \"{}\": {}", path.string(), e.what());
		return NOT_OK;
	}
}

ModuleManagerLibraryHandler::~ModuleManagerLibraryHandler() {
	if(module_ != nullptr) {
		dlclose(module_);
		module_ == nullptr;
	}
}

int ModuleManagerLibraryHandler::getModuleNumber() {
	return getModuleNumber_();
}

int ModuleManagerLibraryHandler::isDeviceTypeSupported(unsigned int device_type) {
	return isDeviceTypeSupported_(device_type);
}

int ModuleManagerLibraryHandler::sendStatusCondition(const struct buffer current_status, const struct buffer new_status,
													 unsigned int device_type) {
	return sendStatusCondition_(current_status, new_status, device_type);
}

int ModuleManagerLibraryHandler::generateCommand(struct buffer *generated_command, const struct buffer new_status,
												 const struct buffer current_status,
												 const struct buffer current_command, unsigned int device_type) {
	return generateCommand_(generated_command, new_status, current_status, current_command, device_type);
}

int ModuleManagerLibraryHandler::aggregateStatus(struct buffer *aggregated_status, const struct buffer current_status,
												 const struct buffer new_status, unsigned int device_type) {
	return aggregateStatus_(aggregated_status, current_status, new_status, device_type);
}

int ModuleManagerLibraryHandler::aggregateError(struct buffer *error_message, const struct buffer current_error_message,
												const struct buffer status, unsigned int device_type) {
	return aggregateError_(error_message, current_error_message, status, device_type);
}

int ModuleManagerLibraryHandler::generateFirstCommand(struct buffer *default_command, unsigned int device_type) {
	return generateFirstCommand_(default_command, device_type);
}

int ModuleManagerLibraryHandler::statusDataValid(const struct buffer status, unsigned int device_type) {
	return statusDataValid_(status, device_type);
}

int ModuleManagerLibraryHandler::commandDataValid(const struct buffer command, unsigned int device_type) {
	return commandDataValid_(command, device_type);
}

}