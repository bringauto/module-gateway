#include <bringauto/modules/ModuleManagerLibraryHandlerAsync.hpp>

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
		moduleBinaryProcess_.wait();
	}
}

void ModuleManagerLibraryHandlerAsync::loadLibrary(const std::filesystem::path &path) {
	moduleBinaryProcess_ = boost::process::child { moduleBinaryPath_.string(), "-m", path.string() };
	if (!moduleBinaryProcess_.valid()) {
		throw std::runtime_error { "Failed to start module binary " + moduleBinaryPath_.string() };
	}
	std::this_thread::sleep_for(std::chrono::seconds(1)); // TODO Not sure how much time is needed.
}

int ModuleManagerLibraryHandlerAsync::getModuleNumber() {
	std::lock_guard lock { getModuleNumberMutex_ };
	return aeronClient.callFunc(fleet_protocol::cxx::getModuleNumberAsync).value_or(NOT_OK);
}

int ModuleManagerLibraryHandlerAsync::isDeviceTypeSupported(unsigned int device_type) {
	std::lock_guard lock { isDeviceTypeSupportedMutex_ };
	return aeronClient.callFunc(fleet_protocol::cxx::isDeviceTypeSupportedAsync,
								device_type).value_or(NOT_OK);
}

int ModuleManagerLibraryHandlerAsync::sendStatusCondition(const Buffer &current_status,
														  const Buffer &new_status,
													 	  unsigned int device_type) {
	std::lock_guard lock { isDeviceTypeSupportedMutex_ };
	fleet_protocol::cxx::ConvertibleBuffer current_status_raw_buffer;
	fleet_protocol::cxx::ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	return aeronClient.callFunc(fleet_protocol::cxx::sendStatusConditionAsync,
								current_status_raw_buffer,
								new_status_raw_buffer,
								device_type).value_or(NOT_OK);
}

int ModuleManagerLibraryHandlerAsync::generateCommand(Buffer &generated_command,
													  const Buffer &new_status,
													  const Buffer &current_status,
													  const Buffer &current_command, unsigned int device_type) {
	std::lock_guard lock { generateCommandMutex_ };
	fleet_protocol::cxx::ConvertibleBuffer new_status_raw_buffer;
	fleet_protocol::cxx::ConvertibleBuffer current_status_raw_buffer;
	fleet_protocol::cxx::ConvertibleBuffer current_command_raw_buffer;

	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}
	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (current_command.isAllocated()) {
		current_command_raw_buffer = current_command.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(fleet_protocol::cxx::generateCommandAsync,
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
	fleet_protocol::cxx::ConvertibleBuffer current_status_raw_buffer;
	fleet_protocol::cxx::ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(fleet_protocol::cxx::aggregateStatusAsync,
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
	fleet_protocol::cxx::ConvertibleBuffer current_error_raw_buffer;
	fleet_protocol::cxx::ConvertibleBuffer status_raw_buffer;

	if (current_error_message.isAllocated()) {
		current_error_raw_buffer = current_error_message.getStructBuffer();
	}
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(fleet_protocol::cxx::aggregateErrorAsync,
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
	auto ret = aeronClient.callFunc(fleet_protocol::cxx::generateFirstCommandAsync, device_type);
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
	fleet_protocol::cxx::ConvertibleBuffer status_raw_buffer;
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}

	return aeronClient.callFunc(fleet_protocol::cxx::statusDataValidAsync,
								status_raw_buffer,
								device_type).value_or(NOT_OK);
}

int ModuleManagerLibraryHandlerAsync::commandDataValid(const Buffer &command, unsigned int device_type) {
	std::lock_guard lock { commandDataValidMutex_ };
	fleet_protocol::cxx::ConvertibleBuffer command_raw_buffer;
	if (command.isAllocated()) {
		command_raw_buffer = command.getStructBuffer();
	}

	return aeronClient.callFunc(fleet_protocol::cxx::commandDataValidAsync,
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

Buffer ModuleManagerLibraryHandlerAsync::constructBuffer(std::size_t size) {
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

Buffer ModuleManagerLibraryHandlerAsync::constructBufferByTakeOwnership(struct ::buffer &buffer) {
	if (buffer.data == nullptr) {
		throw Buffer::BufferNotAllocated { "Buffer not allocated - cannot take ownership" };
	}
	return { buffer, deallocate_ };
}

}
