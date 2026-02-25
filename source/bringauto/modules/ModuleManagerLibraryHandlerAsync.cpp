#include <bringauto/modules/ModuleManagerLibraryHandlerAsync.hpp>
#include <bringauto/settings/LoggerId.hpp>

#include <fleet_protocol/common_headers/general_error_codes.h>

#include <csignal>



namespace bringauto::modules {

ModuleManagerLibraryHandlerAsync::ModuleManagerLibraryHandlerAsync(const std::filesystem::path &moduleBinaryPath, const int moduleNumber) :
		moduleBinaryPath_ { moduleBinaryPath } {
	aeronClient.connect(moduleNumber);
	deallocate_ = [this](struct buffer *buffer) {
		this->deallocate(buffer);
	};
}

ModuleManagerLibraryHandlerAsync::~ModuleManagerLibraryHandlerAsync() {
	if (moduleBinaryProcess_.valid()) {
		::kill(moduleBinaryProcess_.id(), SIGTERM);
		try
		{
			moduleBinaryProcess_.wait();
		}
		catch (const std::exception& e)
		{
			settings::Logger::logError("Failed to wait for module binary process: {}", e.what());
		}
	}
}

void ModuleManagerLibraryHandlerAsync::loadLibrary(const std::filesystem::path &path) {
	moduleBinaryProcess_ = boost::process::child { moduleBinaryPath_.string(), "-m", path.string() };
	if (!moduleBinaryProcess_.valid()) {
		throw std::runtime_error { "Failed to start module binary " + moduleBinaryPath_.string() };
	}
	const auto deadline = std::chrono::steady_clock::now() + settings::AeronClientConstants::aeron_client_startup_timeout;
	while(std::chrono::steady_clock::now() < deadline) {
		if(aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::getModuleNumberAsync).has_value()) {
			return;
		}
	}
	throw std::runtime_error { "Module binary " + moduleBinaryPath_.string() + " did not become ready within startup timeout" };
}

int ModuleManagerLibraryHandlerAsync::getModuleNumber() {
	std::lock_guard lock { getModuleNumberMutex_ };
	return aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::getModuleNumberAsync).value_or(NOT_OK);
}

int ModuleManagerLibraryHandlerAsync::isDeviceTypeSupported(unsigned int device_type) {
	std::lock_guard lock { isDeviceTypeSupportedMutex_ };
	return aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::isDeviceTypeSupportedAsync,
								device_type).value_or(NOT_OK);
}

int ModuleManagerLibraryHandlerAsync::sendStatusCondition(const Buffer &current_status,
														  const Buffer &new_status,
													 	  unsigned int device_type) {
	std::lock_guard lock { sendStatusConditionMutex_ };
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer current_status_raw_buffer;
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	return aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::sendStatusConditionAsync,
								current_status_raw_buffer,
								new_status_raw_buffer,
								device_type).value_or(NOT_OK);
}

int ModuleManagerLibraryHandlerAsync::generateCommand(Buffer &generated_command,
													  const Buffer &new_status,
													  const Buffer &current_status,
													  const Buffer &current_command, unsigned int device_type) {
	std::lock_guard lock { generateCommandMutex_ };
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer new_status_raw_buffer;
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer current_status_raw_buffer;
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer current_command_raw_buffer;

	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}
	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (current_command.isAllocated()) {
		current_command_raw_buffer = current_command.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::generateCommandAsync,
									new_status_raw_buffer,
									current_status_raw_buffer,
									current_command_raw_buffer,
									device_type);

	if (!ret.has_value()) {
		return NOT_OK;
	}

	if (ret.value().returnCode == OK) {
		generated_command = constructBufferByTakeOwnership(ret.value().buffer);
	} else {
		generated_command = constructBuffer();
	}
	return ret.value().returnCode;
}

int ModuleManagerLibraryHandlerAsync::aggregateStatus(Buffer &aggregated_status,
													  const Buffer &current_status,
													  const Buffer &new_status, unsigned int device_type) {
	std::lock_guard lock { aggregateStatusMutex_ };
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer current_status_raw_buffer;
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::aggregateStatusAsync,
									current_status_raw_buffer,
									new_status_raw_buffer,
									device_type);
	if (!ret.has_value()) {
		return NOT_OK;
	}
	if (ret.value().returnCode == OK) {
		aggregated_status = constructBufferByTakeOwnership(ret.value().buffer);
	} else {
		// Needed to properly free the allocated buffer memory
		auto invalid_buffer = constructBufferByTakeOwnership(ret.value().buffer);
		aggregated_status = current_status;
	}
	return ret.value().returnCode;
}

int ModuleManagerLibraryHandlerAsync::aggregateError(Buffer &error_message,
													 const Buffer &current_error_message,
													 const Buffer &status, unsigned int device_type) {
	std::lock_guard lock { aggregateErrorMutex_ };
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer current_error_raw_buffer;
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer status_raw_buffer;

	if (current_error_message.isAllocated()) {
		current_error_raw_buffer = current_error_message.getStructBuffer();
	}
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::aggregateErrorAsync,
									current_error_raw_buffer,
									status_raw_buffer,
									device_type);
	if (!ret.has_value()) {
		return NOT_OK;
	}
	if (ret.value().returnCode == OK) {
		error_message = constructBufferByTakeOwnership(ret.value().buffer);
	} else {
		error_message = constructBuffer();
	}
	return ret.value().returnCode;
}

int ModuleManagerLibraryHandlerAsync::generateFirstCommand(Buffer &default_command, unsigned int device_type) {
	std::lock_guard lock { generateFirstCommandMutex_ };
	auto ret = aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::generateFirstCommandAsync, device_type);
	if (!ret.has_value()) {
		return NOT_OK;
	}
	if (ret.value().returnCode == OK) {
		default_command = constructBufferByTakeOwnership(ret.value().buffer);
	} else {
		default_command = constructBuffer();
	}
	return ret.value().returnCode;
}

int ModuleManagerLibraryHandlerAsync::statusDataValid(const Buffer &status, unsigned int device_type) {
	std::lock_guard lock { statusDataValidMutex_ };
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer status_raw_buffer;
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}

	return aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::statusDataValidAsync,
								status_raw_buffer,
								device_type).value_or(NOT_OK);
}

int ModuleManagerLibraryHandlerAsync::commandDataValid(const Buffer &command, unsigned int device_type) {
	std::lock_guard lock { commandDataValidMutex_ };
	fleet_protocol::async_function_execution_definitions::ConvertibleBuffer command_raw_buffer;
	if (command.isAllocated()) {
		command_raw_buffer = command.getStructBuffer();
	}

	return aeronClient.callFunc(fleet_protocol::async_function_execution_definitions::commandDataValidAsync,
								command_raw_buffer,
								device_type).value_or(NOT_OK);
}

int ModuleManagerLibraryHandlerAsync::allocate(struct buffer *buffer_pointer, size_t size_in_bytes) const {
	try{
		buffer_pointer->data = new char[size_in_bytes]();
	} catch(std::bad_alloc&){
		return NOT_OK;
	}
	buffer_pointer->size_in_bytes = size_in_bytes;
	return OK;
}

void ModuleManagerLibraryHandlerAsync::deallocate(struct buffer *buffer) const {
	delete[] static_cast<char *>(buffer->data);
	buffer->data = nullptr;
	buffer->size_in_bytes = 0;
}

}
