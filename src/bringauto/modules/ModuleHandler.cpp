#include <bringauto/modules/ModuleHandler.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/settings/Constants.hpp>
#include <memory_management.h>



namespace bringauto::modules {

namespace ip = InternalProtocol;
using log = bringauto::logging::Logger;

void ModuleHandler::destroy() {
	log::logInfo("Module handler stopped");
	for(auto it = modules_.begin(); it != modules_.end(); it++)
		it->second.destroy_status_aggregator();
}

void ModuleHandler::run() {
	log::logInfo("Module handler started");
	init_modules();
	handle_messages();
}

void ModuleHandler::init_modules() {
    for(auto const & path: context_->moduleLibraries){
	    init_module(path.second);
    }
}

void ModuleHandler::init_module(const bringauto::modules::ModuleManagerLibraryHandler &libraryHandler) {
	StatusAggregator status_agg {};
	status_agg.init_status_aggregator(libraryHandler);
	log::logInfo("Module with number: {} started", status_agg.get_module_number());
	modules_.insert({ status_agg.get_module_number(), status_agg });
}

void ModuleHandler::handle_messages() {
	while(not context_->ioContext.stopped()) {
		if(fromInternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			continue;
		}

		auto &message = fromInternalQueue_->front();
		if(message.has_deviceconnect()) {
			handle_connect(message.deviceconnect());
		} else if(message.has_devicestatus()) {
			handle_status(message.devicestatus());
		} else {
			log::logInfo("Received message is not Connect or Status");
		}

		fromInternalQueue_->pop();
	}
}

void ModuleHandler::handle_connect(const ip::DeviceConnect &connect) {
	const auto &device = connect.device();
	const auto &module = device.module();
	log::logInfo("Received Connect message from device: {}", device.devicename());

	auto response_type = ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_OK;
	if(not modules_.contains(module)) {
		response_type =
				ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_MODULE_NOT_SUPPORTED;
	} else if(modules_[module].is_device_type_supported(device.devicetype()) == NOT_OK) {
		response_type =
				ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_DEVICE_NOT_SUPPORTED;
	}

	auto response = new ip::DeviceConnectResponse();
	response->set_responsetype(response_type);
	response->set_allocated_device(new ip::Device(device));
	ip::InternalServer msg {};
	msg.set_allocated_deviceconnectresponse(response);
	toInternalQueue_->pushAndNotify(msg);
	log::logInfo("New device {} is trying to connect, sending response {}", device.devicename(), response_type);
}

::device_identification ModuleHandler::mapToDeviceId(const InternalProtocol::Device &device) {
	return ::device_identification { .module = device.module(),
			.device_type = device.devicetype(),
			.device_role = device.devicerole().c_str(),
			.device_name = device.devicename().c_str(),
			.priority = device.priority() };
}

void ModuleHandler::handle_status(const ip::DeviceStatus &status) {
	const auto &device = status.device();
	const auto &moduleNumber = device.module();
	log::logInfo("Received Status message from device: {}", device.devicename());

	if(not modules_.contains(moduleNumber)) {
		log::logWarning("Module number: {} is not supported", moduleNumber);
		return;
	}

	struct ::buffer status_buffer {};
	const auto &statusData = status.statusdata();
	if(allocate(&status_buffer, statusData.size() + 1) == NOT_OK) {
		log::logError("Could not allocate memory for status message");
		return;
	}
	strcpy(static_cast<char *>(status_buffer.data), statusData.c_str());

	const struct ::device_identification deviceId = mapToDeviceId(device);

	int ret = modules_[moduleNumber].add_status_to_aggregator(status_buffer, deviceId);
	if(ret < 0) {
		log::logWarning("Add status to aggregator failed with return code: {}", ret);
		return;
	}

	struct ::buffer command_buffer {};
	ret = modules_[moduleNumber].get_command(status_buffer, deviceId, &command_buffer);
	if(ret != OK) {
		log::logWarning("Retrieving command failed with return code: {}", ret);
		return;
	}

	auto deviceCommand = new ip::DeviceCommand();
	deviceCommand->set_allocated_commanddata(
			new std::string(static_cast<char *>(command_buffer.data), command_buffer.size_in_bytes - 1));
	deviceCommand->set_allocated_device(new InternalProtocol::Device(device));
	ip::InternalServer msg {};
	msg.set_allocated_devicecommand(deviceCommand);
	toInternalQueue_->pushAndNotify(msg);
	log::logInfo("Command succesfully retrieved and sent to device: {}", device.devicename());

	deallocate(&command_buffer);
	deallocate(&status_buffer);
}

}
