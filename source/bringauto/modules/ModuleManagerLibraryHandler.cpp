#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <bringauto/settings/LoggerId.hpp>

#include <stdexcept>
#include <dlfcn.h>



namespace bringauto::modules {

template <typename T>
struct FunctionTypeDeducer;

template <typename R, typename ...Args>
struct FunctionTypeDeducer<std::function<R(Args...)>> {
	using fncptr = R (*)(Args...);
};

using log = bringauto::settings::Logger;

ModuleManagerLibraryHandler::~ModuleManagerLibraryHandler() {
	if(module_ != nullptr) {
		dlclose(module_);
		module_ = nullptr;
	}
}

void ModuleManagerLibraryHandler::loadLibrary(const std::filesystem::path &path) {
	module_ = dlmopen(LM_ID_NEWLM, path.c_str(), RTLD_LAZY);
	if(module_ == nullptr) {
		throw std::runtime_error {"Unable to load library " + path.string() + dlerror()};
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
	log::logDebug("Library " + path.string() + " was successfully loaded");
}

void *ModuleManagerLibraryHandler::checkFunction(const char *functionName) {
	auto function = dlsym(module_, functionName);
	if(not function) {
		throw std::runtime_error {"Function " + std::string(functionName) + " is not included in library"};
	}
	return function;
}

int ModuleManagerLibraryHandler::getModuleNumber() {
	return getModuleNumber_();
}

int ModuleManagerLibraryHandler::isDeviceTypeSupported(unsigned int device_type) {
	return isDeviceTypeSupported_(device_type);
}

int ModuleManagerLibraryHandler::sendStatusCondition(const Buffer current_status,
													 const Buffer new_status,
													 unsigned int device_type) {
	struct ::buffer current_status_raw_buffer {};
	struct ::buffer new_status_raw_buffer {};

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	return sendStatusCondition_(current_status_raw_buffer, new_status_raw_buffer, device_type);
}

int ModuleManagerLibraryHandler::generateCommand(Buffer &generated_command,
												 const Buffer new_status,
												 const Buffer current_status,
												 const Buffer current_command, unsigned int device_type) {
	struct ::buffer raw_buffer {};
	struct ::buffer new_status_raw_buffer {};
	struct ::buffer current_status_raw_buffer {};
	struct ::buffer current_command_raw_buffer {};

	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}
	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (current_command.isAllocated()) {
		current_command_raw_buffer = current_command.getStructBuffer();
	}

	int ret = generateCommand_(&raw_buffer, new_status_raw_buffer,
		current_status_raw_buffer, current_command_raw_buffer, device_type);
	if (ret == OK) {
		generated_command = constructBufferByTakeOwnership(raw_buffer);
	} else {
		generated_command = constructBuffer();
	}
	return ret;
}

int ModuleManagerLibraryHandler::aggregateStatus(Buffer &aggregated_status,
												 const Buffer current_status,
												 const Buffer new_status, unsigned int device_type) {
	struct ::buffer raw_buffer {};
	struct ::buffer current_status_raw_buffer {};
	struct ::buffer new_status_raw_buffer {};

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	int ret = aggregateStatus_(&raw_buffer, current_status_raw_buffer, new_status_raw_buffer, device_type);
	if (ret == OK) {
		aggregated_status = constructBufferByTakeOwnership(raw_buffer);
	} else {
		aggregated_status = current_status;
	}
	return ret;
}

int ModuleManagerLibraryHandler::aggregateError(Buffer &error_message,
												const Buffer current_error_message,
												const Buffer status, unsigned int device_type) {

	struct ::buffer raw_buffer {};
	struct ::buffer current_error_raw_buffer {};
	struct ::buffer status_raw_buffer {};

	if (current_error_message.isAllocated()) {
		current_error_raw_buffer = current_error_message.getStructBuffer();
	}
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}

	int ret = aggregateError_(&raw_buffer, current_error_raw_buffer, status_raw_buffer, device_type);
	if (ret == OK) {
		error_message = constructBufferByTakeOwnership(raw_buffer);
	} else {
		error_message = constructBuffer();
	}
	return ret;
}

int ModuleManagerLibraryHandler::generateFirstCommand(Buffer &default_command, unsigned int device_type) {
	struct ::buffer raw_buffer {};
	int ret = generateFirstCommand_(&raw_buffer, device_type);
	if (ret == OK) {
		default_command = constructBufferByTakeOwnership(raw_buffer);
	} else {
		default_command = constructBuffer();
	}
	return ret;
}

int ModuleManagerLibraryHandler::statusDataValid(const Buffer status, unsigned int device_type) {
	struct ::buffer raw_buffer {};
	if (status.isAllocated()) {
		raw_buffer = status.getStructBuffer();
	}
	return statusDataValid_(raw_buffer, device_type);
}

int ModuleManagerLibraryHandler::commandDataValid(const Buffer command, unsigned int device_type) {
	struct ::buffer raw_buffer {};
	if (command.isAllocated()) {
		raw_buffer = command.getStructBuffer();
	}
	return commandDataValid_(raw_buffer, device_type);
}

int ModuleManagerLibraryHandler::allocate(struct buffer *buffer_pointer, size_t size_in_bytes){
	return allocate_(buffer_pointer, size_in_bytes);
}

void ModuleManagerLibraryHandler::deallocate(struct buffer *buffer){
	deallocate_(buffer);
}

Buffer ModuleManagerLibraryHandler::constructBuffer(std::size_t size) {
	if (size == 0) {
		return Buffer {};
	}
	struct ::buffer buff {};
	buff.size_in_bytes = size;
	if(allocate(&buff, size) != OK) {
		throw std::bad_alloc {};
	}
	return { buff, deallocate_ };
}

Buffer ModuleManagerLibraryHandler::constructBufferByTakeOwnership(struct ::buffer &buffer) {
	if (buffer.data == nullptr) {
		throw Buffer::BufferNotAllocated { "Buffer not allocated - cannot take ownership" };
	}
	return { buffer, deallocate_ };
}

}
