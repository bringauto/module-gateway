#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>
#include <bringauto/settings/LoggerId.hpp>
#include <bringauto/settings/Constants.hpp>

#include <bringauto/fleet_protocol/cxx/StringAsBuffer.hpp>
#include <fleet_protocol/common_headers/general_error_codes.h>

#include <stdexcept>
#include <dlfcn.h>



namespace bringauto::modules {

template <typename T>
struct FunctionTypeDeducer;

template <typename R, typename ...Args>
struct FunctionTypeDeducer<std::function<R(Args...)>> {
	using fncptr = R (*)(Args...);
};

using log = settings::Logger;

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

	if (aeronClient_ != nullptr) {
		aeronClient_->addModule(getModuleNumber_());
	}

	log::logDebug("Library " + path.string() + " was successfully loaded");
}

void *ModuleManagerLibraryHandler::checkFunction(const char *functionName) const {
	const auto function = dlsym(module_, functionName);
	if(not function) {
		throw std::runtime_error {"Function " + std::string(functionName) + " is not included in library"};
	}
	return function;
}

int ModuleManagerLibraryHandler::getModuleNumber() const {
	return getModuleNumber_();
}

int ModuleManagerLibraryHandler::isDeviceTypeSupported(unsigned int device_type) const {
	return isDeviceTypeSupported_(device_type);
}

int ModuleManagerLibraryHandler::sendStatusCondition(const Buffer &current_status,
													 const Buffer &new_status,
													 unsigned int device_type) const {
	struct ::buffer current_status_raw_buffer {};
	struct ::buffer new_status_raw_buffer {};

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	if (aeronClient_ != nullptr) {
		aeronClient_->callModuleFunction(
			aeron_communication::AeronClient::ModuleFunctions::SEND_STATUS_CONDITION,
			constructAeronMessage({&current_status_raw_buffer, &new_status_raw_buffer}, device_type)
		);
		return std::stoi(std::string(aeronClient_->getMessage()));
	}
	return sendStatusCondition_(current_status_raw_buffer, new_status_raw_buffer, device_type);
}

int ModuleManagerLibraryHandler::generateCommand(Buffer &generated_command,
												 const Buffer &new_status,
												 const Buffer &current_status,
												 const Buffer &current_command, unsigned int device_type) {
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

	int ret;
	if (aeronClient_ != nullptr) {
		aeronClient_->callModuleFunction(
			aeron_communication::AeronClient::ModuleFunctions::GENERATE_COMMAND,
			constructAeronMessage({&new_status_raw_buffer, &current_status_raw_buffer, &current_command_raw_buffer}, device_type)
		);
		ret = parseAeronResponse(raw_buffer, aeronClient_->getMessage());
	} else {
		ret = generateCommand_(&raw_buffer, new_status_raw_buffer,
			current_status_raw_buffer, current_command_raw_buffer, device_type);
	}
	if (ret == OK) {
		generated_command = constructBufferByTakeOwnership(raw_buffer);
	} else {
		generated_command = constructBuffer();
	}
	return ret;
}

int ModuleManagerLibraryHandler::aggregateStatus(Buffer &aggregated_status,
												 const Buffer &current_status,
												 const Buffer &new_status, unsigned int device_type) {
	struct ::buffer raw_buffer {};
	struct ::buffer current_status_raw_buffer {};
	struct ::buffer new_status_raw_buffer {};

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	int ret;
	if (aeronClient_ != nullptr) {
		aeronClient_->callModuleFunction(
			aeron_communication::AeronClient::ModuleFunctions::AGGREGATE_STATUS,
			constructAeronMessage({&current_status_raw_buffer, &new_status_raw_buffer}, device_type)
		);
		ret = parseAeronResponse(raw_buffer, aeronClient_->getMessage());
	} else {
		ret = aggregateStatus_(&raw_buffer, current_status_raw_buffer, new_status_raw_buffer, device_type);
	}
	if (ret == OK) {
		aggregated_status = constructBufferByTakeOwnership(raw_buffer);
	} else {
		aggregated_status = current_status;
	}
	return ret;
}

int ModuleManagerLibraryHandler::aggregateError(Buffer &error_message,
												const Buffer &current_error_message,
												const Buffer &status, unsigned int device_type) {

	struct ::buffer raw_buffer {};
	struct ::buffer current_error_raw_buffer {};
	struct ::buffer status_raw_buffer {};

	if (current_error_message.isAllocated()) {
		current_error_raw_buffer = current_error_message.getStructBuffer();
	}
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}
	int ret;
	if (aeronClient_ != nullptr) {
		aeronClient_->callModuleFunction(
			aeron_communication::AeronClient::ModuleFunctions::AGGREGATE_ERROR,
			constructAeronMessage({&current_error_raw_buffer, &status_raw_buffer}, device_type)
		);
		ret = parseAeronResponse(raw_buffer, aeronClient_->getMessage());
	} else {
		ret = aggregateError_(&raw_buffer, current_error_raw_buffer, status_raw_buffer, device_type);
	}
	if (ret == OK) {
		error_message = constructBufferByTakeOwnership(raw_buffer);
	} else {
		error_message = constructBuffer();
	}
	return ret;
}

int ModuleManagerLibraryHandler::generateFirstCommand(Buffer &default_command, unsigned int device_type) {
	struct ::buffer raw_buffer {};
	int ret;
	if (aeronClient_ != nullptr) {
		aeronClient_->callModuleFunction(
			aeron_communication::AeronClient::ModuleFunctions::GENERATE_FIRST_COMMAND,
			constructAeronMessage({}, device_type)
		);
		ret = parseAeronResponse(raw_buffer, aeronClient_->getMessage());	
	} else {
		ret = generateFirstCommand_(&raw_buffer, device_type);
	}
	if (ret == OK) {
		default_command = constructBufferByTakeOwnership(raw_buffer);
	} else {
		default_command = constructBuffer();
	}
	return ret;
}

int ModuleManagerLibraryHandler::statusDataValid(const Buffer &status, unsigned int device_type) const {
	struct ::buffer raw_buffer {};
	if (status.isAllocated()) {
		raw_buffer = status.getStructBuffer();
	}
	if (aeronClient_ != nullptr) {
		aeronClient_->callModuleFunction(
			aeron_communication::AeronClient::ModuleFunctions::STATUS_DATA_VALID,
			constructAeronMessage({&raw_buffer}, device_type)
		);
		return std::stoi(std::string(aeronClient_->getMessage()));
	}
	return statusDataValid_(raw_buffer, device_type);
}

int ModuleManagerLibraryHandler::commandDataValid(const Buffer &command, unsigned int device_type) const {
	struct ::buffer raw_buffer {};
	if (command.isAllocated()) {
		raw_buffer = command.getStructBuffer();
	}
	if (aeronClient_ != nullptr) {
		aeronClient_->callModuleFunction(
			aeron_communication::AeronClient::ModuleFunctions::COMMAND_DATA_VALID,
			constructAeronMessage({&raw_buffer}, device_type)
		);
		return std::stoi(std::string(aeronClient_->getMessage()));
	}
	return commandDataValid_(raw_buffer, device_type);
}

int ModuleManagerLibraryHandler::allocate(struct buffer *buffer_pointer, size_t size_in_bytes) const {
	// if (aeronClient_ != nullptr) {
	// 	aeronClient_->callModuleFunction(aeron_communication::AeronClient::ModuleFunctions::ALLOCATE, "TODO");
	// 	if (aeronClient_->getMessage() != "allocate") {
	// 		throw std::runtime_error("AeronClient did not receive the expected message");
	// 	}
	// }
	return allocate_(buffer_pointer, size_in_bytes);
}

void ModuleManagerLibraryHandler::deallocate(struct buffer *buffer) const {
	// if (aeronClient_ != nullptr) {
	// 	aeronClient_->callModuleFunction(aeron_communication::AeronClient::ModuleFunctions::DEALLOCATE, "TODO");
	// 	if (aeronClient_->getMessage() != "deallocate") {
	// 		throw std::runtime_error("AeronClient did not receive the expected message");
	// 	}
	// }
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

std::string ModuleManagerLibraryHandler::constructAeronMessage(const std::vector<struct ::buffer *> &buffers, int deviceType) const {
	std::string message;
	for (const auto &buff : buffers) {
		message += std::string(static_cast<char*>(buff->data), buff->size_in_bytes) + std::string(settings::Constants::SEPARATOR);
	}
	return message + std::to_string(deviceType);
}

int ModuleManagerLibraryHandler::parseAeronResponse(struct ::buffer &raw_buffer, std::string_view response) const {
	size_t sepPos = response.find(settings::Constants::SEPARATOR);
	if (sepPos == std::string::npos) {
		throw std::runtime_error("Invalid response format: " + std::string(response));
	}
	bringauto::fleet_protocol::cxx::StringAsBuffer::createBufferAndCopyData(
		&raw_buffer,
		response.substr(sepPos + settings::Constants::SEPARATOR.size())
	);
	return std::stoi(std::string(response.substr(0, sepPos)));
}

}
