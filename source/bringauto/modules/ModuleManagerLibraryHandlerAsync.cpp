#include <bringauto/modules/ModuleManagerLibraryHandlerAsync.hpp>
#include <bringauto/settings/Constants.hpp>

#include <fleet_protocol/common_headers/general_error_codes.h>

#include <chrono>
#include <memory>
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

ModuleManagerLibraryHandlerAsync::ModuleManagerLibraryHandlerAsync(const std::string &moduleBinaryPath) :
		moduleBinaryPath_ { moduleBinaryPath } {
	aeronClient.connect();
}

ModuleManagerLibraryHandlerAsync::~ModuleManagerLibraryHandlerAsync() {
	try {
		if (moduleBinaryProcess_.valid()) {
			::kill(moduleBinaryProcess_.id(), SIGTERM);
			moduleBinaryProcess_.wait();
		}
	} catch (...) {}
}

void ModuleManagerLibraryHandlerAsync::loadLibrary(const std::filesystem::path &path) {
	moduleBinaryProcess_ = boost::process::child { moduleBinaryPath_, "-m", path.string() };
	if (!moduleBinaryProcess_.valid()) {
		throw std::runtime_error { "Failed to start module binary " + moduleBinaryPath_ };
	}
	const auto deadline = std::chrono::steady_clock::now() + settings::AeronClientConstants::aeron_client_startup_timeout;
	while(!aeronClient.callFunc(getModuleNumberAsync).has_value()) {
		if(!moduleBinaryProcess_.running()) {
			throw std::runtime_error { "Module binary terminated before becoming ready: " + moduleBinaryPath_ };
		}
		if(std::chrono::steady_clock::now() >= deadline) {
			throw std::runtime_error { "Module binary did not respond within startup timeout: " + moduleBinaryPath_ };
		}
		std::this_thread::sleep_for(settings::AeronClientConstants::module_binary_poll_interval);
	}
}

int ModuleManagerLibraryHandlerAsync::getModuleNumber() const {
	return aeronClient.callFunc(getModuleNumberAsync).value();
}

int ModuleManagerLibraryHandlerAsync::isDeviceTypeSupported(unsigned int device_type) {
	std::lock_guard lock { tmpMutex_ };
	return aeronClient.callFunc(isDeviceTypeSupportedAsync, device_type).value();
}

int ModuleManagerLibraryHandlerAsync::sendStatusCondition(const Buffer &current_status,
														  const Buffer &new_status,
													 	  unsigned int device_type) const {
	ConvertibleBuffer current_status_raw_buffer;
	ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = ConvertibleBuffer{current_status.getStructBuffer()};
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = ConvertibleBuffer{new_status.getStructBuffer()};
	}

	return aeronClient.callFunc(sendStatusConditionAsync, current_status_raw_buffer, new_status_raw_buffer, device_type).value();
}

int ModuleManagerLibraryHandlerAsync::generateCommand(Buffer &generated_command,
													  const Buffer &new_status,
													  const Buffer &current_status,
													  const Buffer &current_command, unsigned int device_type) {
	ConvertibleBuffer new_status_raw_buffer;
	ConvertibleBuffer current_status_raw_buffer;
	ConvertibleBuffer current_command_raw_buffer;

	if (new_status.isAllocated()) {
		new_status_raw_buffer = ConvertibleBuffer{new_status.getStructBuffer()};
	}
	if (current_status.isAllocated()) {
		current_status_raw_buffer = ConvertibleBuffer{current_status.getStructBuffer()};
	}
	if (current_command.isAllocated()) {
		current_command_raw_buffer = ConvertibleBuffer{current_command.getStructBuffer()};
	}

	auto ret = aeronClient.callFunc(generateCommandAsync,
									new_status_raw_buffer,
									current_status_raw_buffer,
									current_command_raw_buffer,
									device_type).value();

	if (ret.returnCode == OK) {
		generated_command = constructBuffer(ret.data);
	} else {
		generated_command = constructBuffer();
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::aggregateStatus(Buffer &aggregated_status,
													  const Buffer &current_status,
													  const Buffer &new_status, unsigned int device_type) {
	ConvertibleBuffer current_status_raw_buffer;
	ConvertibleBuffer new_status_raw_buffer;

	if (current_status.isAllocated()) {
		current_status_raw_buffer = ConvertibleBuffer{current_status.getStructBuffer()};
	}
	if (new_status.isAllocated()) {
		new_status_raw_buffer = ConvertibleBuffer{new_status.getStructBuffer()};
	}

	auto ret = aeronClient.callFunc(aggregateStatusAsync, current_status_raw_buffer, new_status_raw_buffer, device_type).value();
	if (ret.returnCode == OK) {
		aggregated_status = constructBuffer(ret.data);
	} else {
		aggregated_status = current_status;
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::aggregateError(Buffer &error_message,
													 const Buffer &current_error_message,
													 const Buffer &status, unsigned int device_type) {
	ConvertibleBuffer current_error_raw_buffer;
	ConvertibleBuffer status_raw_buffer;

	if (current_error_message.isAllocated()) {
		current_error_raw_buffer = ConvertibleBuffer{current_error_message.getStructBuffer()};
	}
	if (status.isAllocated()) {
		status_raw_buffer = ConvertibleBuffer{status.getStructBuffer()};
	}

	auto ret = aeronClient.callFunc(aggregateErrorAsync, current_error_raw_buffer, status_raw_buffer, device_type).value();
	if (ret.returnCode == OK) {
		error_message = constructBuffer(ret.data);
	} else {
		error_message = constructBuffer();
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::generateFirstCommand(Buffer &default_command, unsigned int device_type) {
	auto ret = aeronClient.callFunc(generateFirstCommandAsync, device_type).value();
	if (ret.returnCode == OK) {
		default_command = constructBuffer(ret.data);
	} else {
		default_command = constructBuffer();
	}
	return ret.returnCode;
}

int ModuleManagerLibraryHandlerAsync::statusDataValid(const Buffer &status, unsigned int device_type) const {
	ConvertibleBuffer status_raw_buffer;
	if (status.isAllocated()) {
		status_raw_buffer = ConvertibleBuffer{status.getStructBuffer()};
	}

	return aeronClient.callFunc(statusDataValidAsync, status_raw_buffer, device_type).value();
}

int ModuleManagerLibraryHandlerAsync::commandDataValid(const Buffer &command, unsigned int device_type) const {
	ConvertibleBuffer command_raw_buffer;
	if (command.isAllocated()) {
		command_raw_buffer = ConvertibleBuffer{command.getStructBuffer()};
	}

	return aeronClient.callFunc(commandDataValidAsync, command_raw_buffer, device_type).value();
}

Buffer ModuleManagerLibraryHandlerAsync::constructBuffer(std::size_t size) {
	if(size == 0) {
		return Buffer {};
	}
	auto data = std::make_unique<char[]>(size);
	struct ::buffer buff { .data = data.release(), .size_in_bytes = size };
	return { buff, [](struct ::buffer *b) {
		std::unique_ptr<char[]> { static_cast<char *>(b->data) };
		b->data = nullptr;
		b->size_in_bytes = 0;
	}};
}

Buffer ModuleManagerLibraryHandlerAsync::constructBuffer(std::span<const uint8_t> data) {
	auto buff = constructBuffer(data.size());
	std::memcpy(buff.getStructBuffer().data, data.data(), data.size());
	return buff;
}

}
