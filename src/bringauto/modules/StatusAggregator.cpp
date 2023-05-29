#include <dlfcn.h>
#include <sstream>

#include <bringauto/logging/Logger.hpp>
#include <bringauto/modules/StatusAggregator.hpp>



namespace bringauto::modules {

template <typename T>
struct FunctionTypeDeducer;

template<typename R, typename ...Args>
struct FunctionTypeDeducer<std::function<R(Args...)>>{
    using fncptr = R (*)(Args...);
};

using log = bringauto::logging::Logger;

std::string StatusAggregator::getId(const ::device_identification &device) {
	std::stringstream ss;
	ss << device.module << "/" << device.device_type << "/" << device.device_role << "/" << device.device_name;
	return ss.str();
}

int StatusAggregator::init_status_aggregator(std::filesystem::path path) {
	module = dlopen(path.c_str(), RTLD_LAZY | RTLD_DEEPBIND);
	if(module == nullptr) {
		return NOT_OK;
	}
	isDeviceTypeSupported = reinterpret_cast<FunctionTypeDeducer<decltype(isDeviceTypeSupported)>::fncptr>(dlsym(module, "is_device_type_supported"));
	getModuleNumber = reinterpret_cast<FunctionTypeDeducer<decltype(getModuleNumber)>::fncptr>(dlsym(module, "get_module_number"));
	generateFirstCommand = reinterpret_cast<FunctionTypeDeducer<decltype(generateFirstCommand)>::fncptr>(dlsym(module, "generate_first_command"));
	statusDataValid = reinterpret_cast<FunctionTypeDeducer<decltype(statusDataValid)>::fncptr>(dlsym(module, "status_data_valid"));
	commandDataValid = reinterpret_cast<FunctionTypeDeducer<decltype(commandDataValid)>::fncptr>(dlsym(module, "command_data_valid"));
	sendStatusCondition = reinterpret_cast<FunctionTypeDeducer<decltype(sendStatusCondition)>::fncptr>(dlsym(module, "send_status_condition"));
	aggregateStatus = reinterpret_cast<FunctionTypeDeducer<decltype(aggregateStatus)>::fncptr>(dlsym(module, "aggregate_status"));
	generateCommand = reinterpret_cast<FunctionTypeDeducer<decltype(generateCommand)>::fncptr>(dlsym(module, "generate_command"));
	return OK;
}

int StatusAggregator::destroy_status_aggregator() {
	if(module == nullptr) {
		return OK;
	}
	dlclose(module);
	module = nullptr;
	clear_all_devices();
	return OK;
}

int StatusAggregator::clear_all_devices() {
	for (auto& [key, device] : devices){
        auto &aggregatedMessages = device.aggregatedMessages;
        while(not aggregatedMessages.empty()){
            auto message = aggregatedMessages.front();
            if (message.data != nullptr){
                deallocate(&message);
            }
            aggregatedMessages.pop();
        }
	    deallocate(&device.status);
	    deallocate(&device.command);
	}
	devices.clear();
	return OK;
}

int StatusAggregator::clear_device(const struct ::device_identification device) {
	std::string id = getId(device);
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	auto &aggregatedMessages = devices[id].aggregatedMessages;
	while(not aggregatedMessages.empty()) {
		aggregatedMessages.pop();
	}
	return OK;
}

int StatusAggregator::remove_device(const struct ::device_identification device) {
	std::string id = getId(device);
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	devices.erase(id);
	return OK;
}

int StatusAggregator::add_status_to_aggregator(const struct ::buffer status,
											   const struct ::device_identification device) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}
	if(status.size_in_bytes == 0 || statusDataValid(status, device_type) == NOT_OK) {
		log::logWarning("Invalid status data");
		return NOT_OK;
	}

	std::string id = getId(device);
	if(not devices.contains(id)) {
		struct buffer commandBuffer {};
		generateFirstCommand(&commandBuffer, device_type);
		struct buffer buf {};
        allocate(&buf, status.size_in_bytes);
        strncpy(static_cast<char *>(buf.data), static_cast<char *>(status.data), status.size_in_bytes -1);
		devices.insert({ id, { commandBuffer, buf }});
		return 0;
	}

	auto &currStatus = devices[id].status;
	auto &aggregatedMessages = devices[id].aggregatedMessages;
	if(sendStatusCondition(currStatus, status, device_type) == OK) {
		struct buffer aggregatedStatusBuff {};
		aggregateStatus(&aggregatedStatusBuff, currStatus, status, device_type);
        deallocate(&currStatus);
        currStatus = aggregatedStatusBuff;
	} else {
		aggregatedMessages.push(currStatus);
        struct buffer buf {};
        allocate(&buf, status.size_in_bytes);
        strncpy(static_cast<char *>(buf.data), static_cast<char *>(status.data), status.size_in_bytes - 1);
		currStatus = buf;
	}

	return aggregatedMessages.size();
}

int StatusAggregator::get_aggregated_status(struct ::buffer *generated_status,
											const struct ::device_identification device) {
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}

	std::string id = getId(device);
	auto &aggregatedMessages = devices[id].aggregatedMessages;
	if(aggregatedMessages.size() == 0) {
		return NO_MESSAGE_AVAILABLE;
	}

	auto status = aggregatedMessages.front();
	aggregatedMessages.pop();
	generated_status->data = status.data;
	generated_status->size_in_bytes = status.size_in_bytes;
	return OK;
}

int StatusAggregator::get_unique_devices(struct ::buffer *unique_devices_buffer) {
    std::stringstream output {};
	for(auto const &[key, value]: devices) {
        output << key << ",";
	}
    output.seekp(-1, std::ios_base::end);
	unique_devices_buffer->data = static_cast<void *>(new std::string(output.str()));
	return devices.size();
}

int StatusAggregator::force_aggregation_on_device(const struct ::device_identification device) {
	std::string id = getId(device);
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}

	auto &aggregatedMessages = devices[id].aggregatedMessages;
	aggregatedMessages.push(devices[id].status);
	return aggregatedMessages.size();
}

int StatusAggregator::is_device_valid(const struct ::device_identification device) {
	if(is_device_type_supported(device.device_type) == OK && devices.contains(getId(device))) {
		return OK;
	}
	return NOT_OK;
}

int StatusAggregator::get_module_number() { return getModuleNumber(); }

int StatusAggregator::update_command(const struct ::buffer command, const struct ::device_identification device) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}

	std::string id = getId(device);
	if(not devices.contains(id)) {
		return DEVICE_NOT_REGISTERED;
	}

    if(commandDataValid(command, device_type) == NOT_OK){
        log::logWarning("Invalid status data");
		return COMMAND_INVALID;
    }

	auto &currCommand = devices[id].command;
	deallocate(&currCommand);
	currCommand = command;
	return OK;
}

int StatusAggregator::get_command(const struct ::buffer status, const struct ::device_identification device,
								  struct ::buffer *command) {
	const auto &device_type = device.device_type;
	if(is_device_type_supported(device_type) == NOT_OK) {
		log::logError("Device type {} is not supported", device_type);
		return DEVICE_NOT_SUPPORTED;
	}

	if(status.size_in_bytes == 0) {
		return STATUS_INVALID;
	}

	std::string id = getId(device);
	auto &currStatus = devices[id].status;
	auto &currCommand = devices[id].command;

	struct buffer generatedCommandBuffer {};
	generateCommand(&generatedCommandBuffer, status, currStatus, currCommand, device_type);
	deallocate(&currCommand);
	currCommand = generatedCommandBuffer;

	int currCommandSize = currCommand.size_in_bytes;
	if(allocate(command, currCommandSize) == NOT_OK) {
		log::logError("Could not allocate memory for command message");
		return NOT_OK;
	}
	memcpy(command->data, currCommand.data, currCommandSize);
	command->size_in_bytes = currCommandSize;

	return OK;
}

int StatusAggregator::is_device_type_supported(unsigned int device_type) { return isDeviceTypeSupported(device_type); }
}