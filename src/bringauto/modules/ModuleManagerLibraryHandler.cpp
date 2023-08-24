#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <bringauto/logging/Logger.hpp>
#include <general_error_codes.h>

#include <stdexcept>
#include <dlfcn.h>



namespace bringauto::modules {

template <typename T>
struct FunctionTypeDeducer;

template <typename R, typename ...Args>
struct FunctionTypeDeducer<std::function<R(Args...)>> {
	using fncptr = R (*)(Args...);
};

using log = bringauto::logging::Logger;

ModuleManagerLibraryHandler::~ModuleManagerLibraryHandler() {
	if(module_ != nullptr) {
		dlclose(module_);
		module_ = nullptr;
	}
}

void ModuleManagerLibraryHandler::loadLibrary(const std::filesystem::path &path) {
	module_ = dlopen(path.c_str(), RTLD_LAZY);
	if(module_ == nullptr) {
		throw std::runtime_error("Unable to load library " + path.string());
	}
	isDeviceTypeSupported_ = reinterpret_cast<FunctionTypeDeducer<decltype(isDeviceTypeSupported_)>::fncptr>(checkFunction(
			"is_device_type_supported"));
	getModuleNumber_ = reinterpret_cast<FunctionTypeDeducer<decltype(getModuleNumber_)>::fncptr>(checkFunction(
			"get_module_number"));
	generateFirstCommand_ = reinterpret_cast<FunctionTypeDeducer<decltype(generateFirstCommand_)>::fncptr>(checkFunction(
			"generate_first_command"));
	statusDataValid_ = reinterpret_cast<FunctionTypeDeducer<decltype(statusDataValid_)>::fncptr>(checkFunction(
			"status_data_valid"));
	commandDataValid_ = reinterpret_cast<FunctionTypeDeducer<decltype(commandDataValid_)>::fncptr>(checkFunction(
			"command_data_valid"));
	sendStatusCondition_ = reinterpret_cast<FunctionTypeDeducer<decltype(sendStatusCondition_)>::fncptr>(checkFunction(
			"send_status_condition"));
	aggregateStatus_ = reinterpret_cast<FunctionTypeDeducer<decltype(aggregateStatus_)>::fncptr>(checkFunction(
			"aggregate_status"));
	generateCommand_ = reinterpret_cast<FunctionTypeDeducer<decltype(generateCommand_)>::fncptr>(checkFunction(
			"generate_command"));
	aggregateError_ = reinterpret_cast<FunctionTypeDeducer<decltype(aggregateError_)>::fncptr>(checkFunction(
			"aggregate_error"));
}

void *ModuleManagerLibraryHandler::checkFunction(const char *functionName) {
	auto function = dlsym(module_, functionName);
	if(not function) {
		throw std::runtime_error("Function " + std::string(functionName) + " is not included in library");
	}
	return function;
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