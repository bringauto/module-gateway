#include <bringauto/modules/ModuleHandler.hpp>
#include <memory_management.h>



namespace bringauto::modules {

const std::chrono::seconds queue_timeout_length { 3 };
namespace ip = InternalProtocol;

void ModuleHandler::destroy() {
	std::cout << "Module handler stopped\n";
	for(auto it = modules_.begin(); it != modules_.end(); it++)
		it->second.destroy_status_aggregator();
}

void ModuleHandler::run() {
	init_modules();
	handle_messages();
}

void ModuleHandler::init_modules() {
	init_module("./libbutton_module.so");
	init_module("./libmission_module.so");
}

void ModuleHandler::init_module(const char *path) {
	StatusAggregator status_agg {};
	status_agg.init_status_aggregator(path);
	std::cout << "Module number: " << status_agg.get_module_number() << "\n";
	modules_.insert({ status_agg.get_module_number(), status_agg });
}

void ModuleHandler::handle_messages() {
	while(not context_->ioContext.stopped()) {
		std::cout << "Waitting for new message\n";
		if(fromInternalQueue_->waitForValueWithTimeout(queue_timeout_length)) {
			std::cout << "Timeout: empty queue\n";
			continue; // queue is empty after timeout so probably disconnect or something(clear all devices?)
		}

		auto &message = fromInternalQueue_->front();

		if(message.has_deviceconnect()) {
			std::cout << "Received Connect message\n";
			handle_connect(message.deviceconnect());
		} else if(message.has_devicestatus()) {
			std::cout << "Received Status message\n";
			handle_status(message.devicestatus());
		} else {
			std::cout << "Received message is not Connect or Status\n";
		}

		fromInternalQueue_->pop();
	}
}

void ModuleHandler::handle_connect(const ip::DeviceConnect &connect) {
	auto response_type = ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_OK;
	const auto &module = connect.device().module();
	if(not modules_.contains(module))
	response_type =
			ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_MODULE_NOT_SUPPORTED;
	else if(modules_[module].is_device_type_supported(connect.device().devicetype()) == NOT_OK)
	response_type =
			ip::DeviceConnectResponse_ResponseType::DeviceConnectResponse_ResponseType_DEVICE_NOT_SUPPORTED;

	auto response = new ip::DeviceConnectResponse();
	response->set_responsetype(response_type);
	response->set_allocated_device(new ip::Device(connect.device()));
	ip::InternalServer msg {};
	msg.set_allocated_deviceconnectresponse(response);
	toInternalQueue_->pushAndNotify(msg);
	std::cout << "New client connected, sending response " << response_type << "\n";
}

::device_identification ModuleHandler::mapToDeviceId(const InternalProtocol::Device &device) {
	return ::device_identification {
			.module = device.module(),
			.device_type = device.devicetype(), // change in device_identification to unsigned int
			.device_role = device.devicerole().c_str(), .device_name = device.devicename().c_str(),
			.priority = device.priority() // change in device_identification to unsigned int
	};
}

void ModuleHandler::handle_status(const ip::DeviceStatus &status) {
	auto &device = status.device();
	const auto &moduleNumber = device.module();
	if(not modules_.contains(moduleNumber)) {
		std::cout << "Module number: " << moduleNumber << " is not supported\n";
		return;
	}

	struct ::buffer status_buffer {};
	status_buffer.data = malloc(status.ByteSizeLong());
	status_buffer.size_in_bytes = status.ByteSizeLong();
	status.SerializeToArray(status_buffer.data, status_buffer.size_in_bytes);

	const struct ::device_identification deviceId = mapToDeviceId(device);

	struct ::buffer command_buffer {};
	command_buffer.data = malloc(40);
	command_buffer.size_in_bytes = 40;

	int ret = modules_[moduleNumber].get_command(status_buffer, deviceId, &command_buffer);
	if(ret <= 0) {
		std::cout << "Retrieving command failed with return code: " << ret << "\n";
		return;
	}

	auto deviceCommand = new ip::DeviceCommand();
	deviceCommand->ParseFromArray(command_buffer.data, command_buffer.size_in_bytes);
	free(command_buffer.data);
	ip::InternalServer msg {};
	msg.set_allocated_devicecommand(deviceCommand);
	toInternalQueue_->pushAndNotify(msg);
	std::cout << "Command succesfully retrieved and sent\n";
}

}
