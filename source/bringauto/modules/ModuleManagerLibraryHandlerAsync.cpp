#include <bringauto/modules/ModuleManagerLibraryHandlerAsync.hpp>
#include <bringauto/settings/Constants.hpp>

#include <fleet_protocol/common_headers/general_error_codes.h>

#include <csignal>



namespace bringauto::modules {

async_function_execution::AsyncFunctionExecutor aeronClient {
	async_function_execution::Config {
		.isProducer = true,
		.defaultTimeout = std::chrono::seconds(1)
	},
	async_function_execution::FunctionList {
		getModuleNumberAsync,
		isDeviceTypeSupportedAsync,
		sendStatusConditionAsync,
		generateCommandAsync,
		aggregateStatusAsync,
		aggregateErrorAsync,
		generateFirstCommandAsync,
		statusDataValidAsync,
		commandDataValidAsync
	}
};

ModuleManagerLibraryHandlerAsync::ModuleManagerLibraryHandlerAsync(const std::filesystem::path &moduleBinaryPath) :
		moduleBinaryPath_ { moduleBinaryPath } {
	aeronClient.connect();
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
	return aeronClient.callFunc(getModuleNumberAsync).value();
}

int ModuleManagerLibraryHandlerAsync::isDeviceTypeSupported(unsigned int device_type) {
	std::lock_guard lock { isDeviceTypeSupportedMutex_ };
	return aeronClient.callFunc(isDeviceTypeSupportedAsync, device_type).value();
}

int ModuleManagerLibraryHandlerAsync::sendStatusCondition(const Buffer &current_status,
														  const Buffer &new_status,
													 	  unsigned int device_type) {
	std::lock_guard lock { isDeviceTypeSupportedMutex_ };
	ConvertibleBuffer current_status_raw_buffer;
	ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	return aeronClient.callFunc(sendStatusConditionAsync, current_status_raw_buffer, new_status_raw_buffer, device_type).value();
}

int ModuleManagerLibraryHandlerAsync::generateCommand(Buffer &generated_command,
													  const Buffer &new_status,
													  const Buffer &current_status,
													  const Buffer &current_command, unsigned int device_type) {
	std::lock_guard lock { generateCommandMutex_ };
	ConvertibleBuffer new_status_raw_buffer;
	ConvertibleBuffer current_status_raw_buffer;
	ConvertibleBuffer current_command_raw_buffer;

	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}
	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (current_command.isAllocated()) {
		current_command_raw_buffer = current_command.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(generateCommandAsync,
									new_status_raw_buffer,
									current_status_raw_buffer,
									current_command_raw_buffer,
									device_type).value();

	if (ret.returnCode == OK) {
		generated_command = constructBufferByTakeOwnership(ret.buffer);
	} else {
		generated_command = constructBuffer();
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::aggregateStatus(Buffer &aggregated_status,
													  const Buffer &current_status,
													  const Buffer &new_status, unsigned int device_type) {
	std::lock_guard lock { aggregateStatusMutex_ };
	ConvertibleBuffer current_status_raw_buffer;
	ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(aggregateStatusAsync, current_status_raw_buffer, new_status_raw_buffer, device_type).value();
	if (ret.returnCode == OK) {
		aggregated_status = constructBufferByTakeOwnership(ret.buffer);
	} else {
		// Needed to properly free the allocated buffer memory
		auto invalid_buffer = constructBufferByTakeOwnership(ret.buffer);
		aggregated_status = current_status;
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::aggregateError(Buffer &error_message,
													 const Buffer &current_error_message,
													 const Buffer &status, unsigned int device_type) {
	std::lock_guard lock { aggregateErrorMutex_ };
	ConvertibleBuffer current_error_raw_buffer;
	ConvertibleBuffer status_raw_buffer;

	if (current_error_message.isAllocated()) {
		current_error_raw_buffer = current_error_message.getStructBuffer();
	}
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(aggregateErrorAsync, current_error_raw_buffer, status_raw_buffer, device_type).value();
	if (ret.returnCode == OK) {
		error_message = constructBufferByTakeOwnership(ret.buffer);
	} else {
		error_message = constructBuffer();
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::generateFirstCommand(Buffer &default_command, unsigned int device_type) {
	std::lock_guard lock { generateFirstCommandMutex_ };
	auto ret = aeronClient.callFunc(generateFirstCommandAsync, device_type).value();
	if (ret.returnCode == OK) {
		default_command = constructBufferByTakeOwnership(ret.buffer);
	} else {
		default_command = constructBuffer();
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::statusDataValid(const Buffer &status, unsigned int device_type) {
	std::lock_guard lock { statusDataValidMutex_ };
	ConvertibleBuffer status_raw_buffer;
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}

	return aeronClient.callFunc(statusDataValidAsync, status_raw_buffer, device_type).value();
}

int ModuleManagerLibraryHandlerAsync::commandDataValid(const Buffer &command, unsigned int device_type) {
	std::lock_guard lock { commandDataValidMutex_ };
	ConvertibleBuffer command_raw_buffer;
	if (command.isAllocated()) {
		command_raw_buffer = command.getStructBuffer();
	}

	return aeronClient.callFunc(commandDataValidAsync, command_raw_buffer, device_type).value();
}

int ModuleManagerLibraryHandlerAsync::allocate(struct buffer *buffer_pointer, size_t size_in_bytes) const {
	return ::allocate(buffer_pointer, size_in_bytes);
}

void ModuleManagerLibraryHandlerAsync::deallocate(struct buffer *buffer) const {
	::deallocate(buffer);
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
