#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <bringauto/logging/Logger.hpp>

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
	module_ = dlmopen(LM_ID_NEWLM, path.c_str(), RTLD_LAZY);
	if(module_ == nullptr) {
		throw std::runtime_error("Unable to load library " + path.string() + dlerror());
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
	allocate_ = reinterpret_cast<FunctionTypeDeducer<decltype(allocate_)>::fncptr>(checkFunction(
			"allocate"));
	deallocate_ = reinterpret_cast<FunctionTypeDeducer<decltype(deallocate_)>::fncptr>(checkFunction(
			"deallocate"));
	log::logDebug("Library " + path.string() + " was succesfully loaded");
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

int ModuleManagerLibraryHandler::sendStatusCondition(const bringauto::modules::Buffer current_status,
													 const bringauto::modules::Buffer new_status,
													 unsigned int device_type) {
	return sendStatusCondition_(current_status.getStructBuffer(), new_status.getStructBuffer(), device_type);
}

int ModuleManagerLibraryHandler::generateCommand(bringauto::modules::Buffer &generated_command,
												 const bringauto::modules::Buffer new_status,
												 const bringauto::modules::Buffer current_status,
												 const bringauto::modules::Buffer current_command, unsigned int device_type) {
	return generateCommand_(&generated_command.raw_buffer_, new_status.getStructBuffer(),
		current_status.getStructBuffer(), current_command.getStructBuffer(), device_type);
}

int ModuleManagerLibraryHandler::aggregateStatus(bringauto::modules::Buffer &aggregated_status,
												 const bringauto::modules::Buffer current_status,
												 const bringauto::modules::Buffer new_status, unsigned int device_type) {
	return aggregateStatus_(&aggregated_status.raw_buffer_, current_status.getStructBuffer(),
		new_status.getStructBuffer(), device_type);
}

int ModuleManagerLibraryHandler::aggregateError(bringauto::modules::Buffer &error_message,
												const bringauto::modules::Buffer current_error_message,
												const bringauto::modules::Buffer status, unsigned int device_type) {
	return aggregateError_(&error_message.raw_buffer_, current_error_message.getStructBuffer(),
		status.getStructBuffer(), device_type);
}

int ModuleManagerLibraryHandler::generateFirstCommand(bringauto::modules::Buffer &default_command, unsigned int device_type) {
	return generateFirstCommand_(&default_command.raw_buffer_, device_type);
}

int ModuleManagerLibraryHandler::statusDataValid(const bringauto::modules::Buffer status, unsigned int device_type) {
	return statusDataValid_(status.getStructBuffer(), device_type);
}

int ModuleManagerLibraryHandler::commandDataValid(const bringauto::modules::Buffer command, unsigned int device_type) {
	return commandDataValid_(command.getStructBuffer(), device_type);
}

int ModuleManagerLibraryHandler::allocate(struct buffer *buffer_pointer, size_t size_in_bytes){
	return allocate_(buffer_pointer, size_in_bytes);
}

void ModuleManagerLibraryHandler::deallocate(struct buffer *buffer){
	deallocate_(buffer);
}

bringauto::modules::Buffer ModuleManagerLibraryHandler::constructBufferByAllocate(std::size_t size) {
	struct ::buffer buff;
	buff.size_in_bytes = size;
	if(allocate(&buff, size) != OK) {
		throw std::bad_alloc();
	}
	return { buff, deallocate_ };
}

bringauto::modules::Buffer ModuleManagerLibraryHandler::constructBufferByTakeOwnership(struct ::buffer &buffer) {
	return { buffer, deallocate_ };
}

}
