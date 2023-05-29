#include <sstream>
#include <dlfcn.h>

#include <bringauto/modules/StatusAggregator.hpp>



namespace bringauto::modules {

std::string StatusAggregator::getId(const ::device_identification &device) {
	std::stringstream ss;
	ss << device.module << "/" << device.device_type << "/" << device.device_role << "/" << device.device_name;
	return ss.str();
}

int StatusAggregator::init_status_aggregator(const char *path) {
	module = dlopen(path, RTLD_LAZY | RTLD_DEEPBIND);
	if(module == nullptr)
	return NOT_OK;
	isDeviceTypeSupported = reinterpret_cast<fnc_int>(dlsym(module, "is_device_type_supported"));
	getModuleNumber = reinterpret_cast<fnc_void>(dlsym(module, "get_module_number"));
	generateFirstCommand = reinterpret_cast<fnc_buffPtr_int>(dlsym(module, "generate_first_command"));
	statusDataValid = reinterpret_cast<fnc_buff_int>(dlsym(module, "status_data_valid"));
	commandDataValid = reinterpret_cast<fnc_buff_int>(dlsym(module, "command_data_valid"));
	aggregateStatus = reinterpret_cast<fnc_buffPtr_buff_buff_int>(dlsym(module, "aggregate_status"));
	generateCommand = reinterpret_cast<fnc_buffPtr_buff_buff_buff_int>(dlsym(module, "generate_command"));
	return OK;
}

int StatusAggregator::destroy_status_aggregator() { /// module specific
	if(module == nullptr)
	return OK;
	dlclose(module);
	module = nullptr;
	return OK;
}

int StatusAggregator::clear_all_devices() {
	devices.clear();
	return OK;
}

int StatusAggregator::clear_device(const struct ::device_identification device) {
	std::string id = getId(device);
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	// struct device_state newDeviceState;
	// std::swap(devices[id], newDeviceState);
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

// if this function is only used in get_command probably remove checks
int
StatusAggregator::add_status_to_aggregator(const struct ::buffer status, const struct ::device_identification device) {
	if(isDeviceTypeSupported(device.device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}
	if(status.size_in_bytes == 0) {
		return NOT_OK;
	}

	auto statusMessage = InternalProtocol::DeviceStatus();
	statusMessage.ParseFromArray(status.data, status.size_in_bytes);

	struct buffer statusBuffer;
	statusBuffer.data = static_cast<void *>(new std::string(statusMessage.statusdata()));
	if(statusDataValid(statusBuffer, device.device_type) == NOT_OK) {
		std::cout << "Invalid status data\n";
		return NOT_OK;
	}

	std::string id = getId(device);
	if(not devices.contains(id)) {
		struct buffer buffer;
		buffer.data = malloc(30*sizeof(char));
		buffer.size_in_bytes = 30;
		generateFirstCommand(&buffer, device.device_type);
		devices.insert({ id, { buffer, statusMessage.device() }});
	}

	devices[id].receivedMessages.push(statusMessage);

	// aggregate
	auto &aggregatedMessages = devices[id].aggregatedMessages;

	return aggregatedMessages.size();
}

int StatusAggregator::get_aggregated_status(struct ::buffer *generated_status,
											const struct ::device_identification device) {
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}
	if(device.module != get_module_number()) // maybe this is not needed
	return DEVICE_NOT_SUPPORTED;

	std::string id = getId(device);
	auto &aggregatedMessages = devices[id].aggregatedMessages;
	if(aggregatedMessages.size() == 0) {
		return NO_MESSAGE_AVAILABLE;
	}

	auto status = aggregatedMessages.front();
	aggregatedMessages.pop();
	if(status.SerializeToArray(generated_status->data, generated_status->size_in_bytes)) {
		return OK;
	}
	return NOT_OK;
}

int StatusAggregator::get_unique_devices(struct ::buffer *unique_devices_buffer) {
	for(auto &item: devices) {
		// struct device_identification device
	}
	return OK;
}

int StatusAggregator::force_aggregation_on_device(const struct ::device_identification device) {
	std::string id = getId(device);
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}

	// aggregate
	auto &aggregatedMessages = devices[id].aggregatedMessages;

	return aggregatedMessages.size();
}

int StatusAggregator::is_device_valid(const struct ::device_identification device) {
	if(isDeviceTypeSupported(device.device_type) == OK && devices.contains(getId(device))) {
		return OK;
	}
	return NOT_OK;
}

int StatusAggregator::get_module_number() {
	return getModuleNumber();
}

int StatusAggregator::update_command(const struct ::buffer command, const struct ::device_identification device) {
	if(device.module != get_module_number())
	return DEVICE_NOT_SUPPORTED;
	if(is_device_valid(device) == NOT_OK) {
		return DEVICE_NOT_REGISTERED;
	}

	InternalProtocol::DeviceCommand newCommand;
	if(not newCommand.ParseFromArray(command.data, command.size_in_bytes)) {
		return COMMAND_INVALID;
	}

	devices[getId(device)].command = newCommand;
	return OK;
}

int StatusAggregator::get_command(const struct ::buffer status, const struct ::device_identification device,
								  struct ::buffer *command) {
	if(isDeviceTypeSupported(device.device_type) == NOT_OK) {
		return DEVICE_NOT_SUPPORTED;
	}

	int ret = add_status_to_aggregator(status, device);
	if(ret != OK) {
		return ret;
	}

	std::string id = getId(device);
	auto &actualCommand = devices[id].command;
	int actualCommandSize = actualCommand.ByteSizeLong();
	if(actualCommandSize > command->size_in_bytes) {
		return BUFFER_TOO_SMALL;
	}
	actualCommand.SerializeToArray(command->data, command->size_in_bytes);

	return actualCommandSize;
}

// this function does not exists in status_aggregator but it needed
int StatusAggregator::is_device_type_supported(int device_type) {
	return isDeviceTypeSupported(device_type);
}
}