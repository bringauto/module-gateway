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
	settings::FunctionIds::functionList
};

ModuleManagerLibraryHandlerAsync::ModuleManagerLibraryHandlerAsync() {
	aeronClient.connect();
	deallocate_ = [this](struct buffer *buffer) {
		this->deallocate(buffer);
	};
}

ModuleManagerLibraryHandlerAsync::~ModuleManagerLibraryHandlerAsync() {
	if (moduleBinaryPid_ != 0) {
		::kill(moduleBinaryPid_, SIGTERM);
	}
}

void ModuleManagerLibraryHandlerAsync::loadLibrary(const std::filesystem::path &path, const std::string &moduleBinaryPath) {
	pid_t pid = ::fork();
	if (pid < 0) {
		throw std::runtime_error { "Fork failed when trying to start module binary " + moduleBinaryPath };
	}
	if (pid > 0) {
		moduleBinaryPid_ = pid;
		return;
	}
	char *args[4];
	args[0] = const_cast<char *>(moduleBinaryPath.c_str());
	args[1] = const_cast<char *>("-m");
	args[2] = const_cast<char *>(path.c_str());
	args[3] = nullptr;
	if (::execv(moduleBinaryPath.c_str(), args) < 0) {
		throw std::runtime_error { "Exec failed when trying to start module binary " + moduleBinaryPath };
	}
}

int ModuleManagerLibraryHandlerAsync::getModuleNumber() const {
	return aeronClient.callFunc(settings::FunctionIds::getModuleNumber);
}

int ModuleManagerLibraryHandlerAsync::isDeviceTypeSupported(unsigned int device_type) const {
	return aeronClient.callFunc(settings::FunctionIds::isDeviceTypeSupported, device_type);
}

int ModuleManagerLibraryHandlerAsync::sendStatusCondition(const Buffer &current_status,
														  const Buffer &new_status,
													 	  unsigned int device_type) const {
	settings::ConvertibleBuffer current_status_raw_buffer;
	settings::ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	return aeronClient.callFunc(settings::FunctionIds::sendStatusCondition, current_status_raw_buffer, new_status_raw_buffer, device_type);
}

int ModuleManagerLibraryHandlerAsync::generateCommand(Buffer &generated_command,
													  const Buffer &new_status,
													  const Buffer &current_status,
													  const Buffer &current_command, unsigned int device_type) {
	settings::ConvertibleBuffer new_status_raw_buffer;
	settings::ConvertibleBuffer current_status_raw_buffer;
	settings::ConvertibleBuffer current_command_raw_buffer;

	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}
	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (current_command.isAllocated()) {
		current_command_raw_buffer = current_command.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(settings::FunctionIds::generateCommand,
									new_status_raw_buffer,
									current_status_raw_buffer,
									current_command_raw_buffer,
									device_type);

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
	settings::ConvertibleBuffer current_status_raw_buffer;
	settings::ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = current_status.getStructBuffer();
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = new_status.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(settings::FunctionIds::aggregateStatus, current_status_raw_buffer, new_status_raw_buffer, device_type);
	if (ret.returnCode == OK) {
		aggregated_status = constructBufferByTakeOwnership(ret.buffer);
	} else {
		aggregated_status = current_status;
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::aggregateError(Buffer &error_message,
													 const Buffer &current_error_message,
													 const Buffer &status, unsigned int device_type) {
	settings::ConvertibleBuffer current_error_raw_buffer;
	settings::ConvertibleBuffer status_raw_buffer;

	if (current_error_message.isAllocated()) {
		current_error_raw_buffer = current_error_message.getStructBuffer();
	}
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}

	auto ret = aeronClient.callFunc(settings::FunctionIds::aggregateError, current_error_raw_buffer, status_raw_buffer, device_type);
	if (ret.returnCode == OK) {
		error_message = constructBufferByTakeOwnership(ret.buffer);
	} else {
		error_message = constructBuffer();
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::generateFirstCommand(Buffer &default_command, unsigned int device_type) {
	auto ret = aeronClient.callFunc(settings::FunctionIds::generateFirstCommand, device_type);
	if (ret.returnCode == OK) {
		default_command = constructBufferByTakeOwnership(ret.buffer);
	} else {
		default_command = constructBuffer();
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::statusDataValid(const Buffer &status, unsigned int device_type) const {
	settings::ConvertibleBuffer status_raw_buffer;
	if (status.isAllocated()) {
		status_raw_buffer = status.getStructBuffer();
	}

	return aeronClient.callFunc(settings::FunctionIds::statusDataValid, status_raw_buffer, device_type);
}

int ModuleManagerLibraryHandlerAsync::commandDataValid(const Buffer &command, unsigned int device_type) const {
	settings::ConvertibleBuffer command_raw_buffer;
	if (command.isAllocated()) {
		command_raw_buffer = command.getStructBuffer();
	}

	return aeronClient.callFunc(settings::FunctionIds::commandDataValid, command_raw_buffer, device_type);
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
