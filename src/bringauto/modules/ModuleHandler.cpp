#include <bringauto/modules/ModuleHandler.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/utils/utils.hpp>
#include <memory_management.h>



namespace bringauto::modules {

namespace ip = InternalProtocol;
using log = bringauto::logging::Logger;

void ModuleHandler::destroy() {
	log::logInfo("Module handler stopped");
}

void ModuleHandler::run() {
	log::logInfo("Module handler started");
	handle_messages();
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
	const auto &moduleNumber = device.module();
    auto &statusAggregators = context_->statusAggregators;
	log::logInfo("Received Connect message from device: {}", device.devicename());

	auto response_type = ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_OK;
	if(not statusAggregators.contains(moduleNumber)) {
		response_type =
				ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_MODULE_NOT_SUPPORTED;
	} else if(statusAggregators[moduleNumber]->is_device_type_supported(device.devicetype()) == NOT_OK) {
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

void ModuleHandler::handle_status(const ip::DeviceStatus &status) {
	const auto &device = status.device();
	const auto &moduleNumber = device.module();
	const auto &deviceName = device.devicename();
    auto &statusAggregators = context_->statusAggregators;
	log::logDebug("Received Status message from device: {}", deviceName);

	if(not statusAggregators.contains(moduleNumber)) {
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

	const struct ::device_identification deviceId = utils::mapToDeviceId(device);

	auto &statusAggregator = statusAggregators[moduleNumber];
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, deviceId);
	if(ret < 0) {
		log::logWarning("Add status to aggregator failed with return code: {}", ret);
		return;
	} else if(ret > 0) {
		struct ::buffer aggregatedStatusBuffer {};
		statusAggregator->get_aggregated_status(&aggregatedStatusBuffer, deviceId);
		auto aggregatedStatus = new ip::DeviceStatus();
		aggregatedStatus->set_allocated_statusdata(new std::string(static_cast<char *>(aggregatedStatusBuffer.data),
																   aggregatedStatusBuffer.size_in_bytes - 1));
		aggregatedStatus->set_allocated_device(new InternalProtocol::Device(device));
		ip::InternalClient msg {};
		msg.set_allocated_devicestatus(aggregatedStatus);
		toExternalQueue_->pushAndNotify(msg);
		deallocate(&aggregatedStatusBuffer);
	}

	struct ::buffer command_buffer {};
	ret = statusAggregator->get_command(status_buffer, deviceId, &command_buffer);
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
	log::logDebug("Command succesfully retrieved and sent to device: {}", deviceName);

	deallocate(&command_buffer);
	deallocate(&status_buffer);
}

}
