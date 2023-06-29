#include <bringauto/common_utils/ProtobufUtils.hpp>

#include <general_error_codes.h>

namespace bringauto::common_utils {

InternalProtocol::InternalServer ProtobufUtils::CreateInternalServerConnectResponseMessage(const InternalProtocol::Device &device,
																						   const InternalProtocol::DeviceConnectResponse_ResponseType &resType) {
	InternalProtocol::InternalServer message;
	auto response = message.mutable_deviceconnectresponse();
	response->set_responsetype(resType);
	auto device_ = response->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalServer ProtobufUtils::CreateInternalServerCommandMessage(const InternalProtocol::Device &device,
																				   const buffer& command) {
	InternalProtocol::InternalServer message;
	auto deviceCommand = message.mutable_devicecommand();
	deviceCommand->set_commanddata(command.data, command.size_in_bytes);
	auto device_ = deviceCommand->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalClient ProtobufUtils::CreateInternalClientStatusMessage(const InternalProtocol::Device &device,
																				  const buffer &status) {
	InternalProtocol::InternalClient message;
	auto deviceStatus = message.mutable_devicestatus();
	deviceStatus->set_statusdata(status.data, status.size_in_bytes);
	auto device_ = deviceStatus->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::DeviceStatus ProtobufUtils::CreateDeviceStatus(const InternalProtocol::Device &device,
																 const buffer &statusData) {
	InternalProtocol::DeviceStatus deviceStatus;
	deviceStatus.mutable_device()->CopyFrom(device);
	deviceStatus.set_statusdata(statusData.data, statusData.size_in_bytes);
	return deviceStatus;
}

InternalProtocol::DeviceStatus ProtobufUtils::CreateDeviceStatus(const device_identification &device,
																 const buffer &statusData) {
	auto deviceMsg = common_utils::ProtobufUtils::CreateDevice(device.module, device.device_type, device.device_role, device.device_name, device.priority);
	return CreateDeviceStatus(deviceMsg, statusData);
}

device_identification ProtobufUtils::ParseDevice(const InternalProtocol::Device &device) {
	return ::device_identification { .module = device.module(),
		.device_type = device.devicetype(),
		.device_role = device.devicerole().c_str(),
		.device_name = device.devicename().c_str(),
		.priority = device.priority() };
}

buffer ProtobufUtils::ProtobufToBuffer(const google::protobuf::Message &protobufMessage) {
	struct buffer message {};
	if ((allocate(&message, protobufMessage.ByteSizeLong())) == OK) {
		protobufMessage.SerializeToArray(message.data, (int)message.size_in_bytes);
	}
	return message;
}

InternalProtocol::Device ProtobufUtils::CreateDevice(const device_identification& device) {
	return CreateDevice(device.module, device.device_type, device.device_role, device.device_name, device.priority);
}

InternalProtocol::Device ProtobufUtils::CreateDevice(const structures::DeviceIdentification &device) {
	return CreateDevice(device.getModule(), device.getDeviceType(), device.getDeviceRole(), device.getDeviceName(), device.getPriority());
}

InternalProtocol::Device ProtobufUtils::CreateDevice(int module, unsigned int type, const std::string &role, const std::string &name, unsigned int priority) {
	InternalProtocol::Device device;
	device.set_module(static_cast<InternalProtocol::Device::Module>(module));
	device.set_devicetype(type);
	device.set_devicerole(role);
	device.set_devicename(name);
	device.set_priority(priority);
	return device;
}

ExternalProtocol::ExternalClient ProtobufUtils::CreateExternalClientConnect(const std::string& sessionId,
																			const std::string& company,
																			const std::string& vehicleName,
																			const std::vector<device_identification> &devices) {
	ExternalProtocol::ExternalClient externalMessage;
	auto connectMessage = externalMessage.mutable_connect();

	connectMessage->set_sessionid(sessionId);
	connectMessage->set_company(company);
	connectMessage->set_vehiclename(vehicleName);

	for(const auto &tmpDevice: devices) {
		auto devicePtr = connectMessage->add_devices();
		devicePtr->CopyFrom(CreateDevice(tmpDevice));
	}

	return externalMessage;
}

ExternalProtocol::ExternalClient ProtobufUtils::CreateExternalClientConnect(const std::string& sessionId,
																			const std::string& company,
																			const std::string& vehicleName,
																			const std::vector<structures::DeviceIdentification> &devices) {
	ExternalProtocol::ExternalClient externalMessage;
	auto connectMessage = externalMessage.mutable_connect();

	connectMessage->set_sessionid(sessionId);
	connectMessage->set_company(company);
	connectMessage->set_vehiclename(vehicleName);

	for(const auto &tmpDevice: devices) {
		auto devicePtr = connectMessage->add_devices();
		devicePtr->CopyFrom(CreateDevice(tmpDevice));
	}

	return externalMessage;
}

ExternalProtocol::ExternalClient ProtobufUtils::CreateExternalClientStatus(const std::string& sessionId,
																		   ExternalProtocol::Status_DeviceState deviceState,
																		   u_int32_t messageCounter,
																		   const InternalProtocol::DeviceStatus& deviceStatus,
																		   const buffer& errorMessage) {
	ExternalProtocol::ExternalClient externalMessage;
	ExternalProtocol::Status* status = externalMessage.mutable_status();
	status->mutable_devicestatus()->CopyFrom(deviceStatus);
	status->set_sessionid(sessionId);
	status->set_devicestate(deviceState);
	status->set_messagecounter(messageCounter);
	if (errorMessage.size_in_bytes > 0 && errorMessage.data != nullptr) {
		status->set_errormessage(errorMessage.data, errorMessage.size_in_bytes);
	}
	return externalMessage;
}

ExternalProtocol::ExternalClient ProtobufUtils::CreateExternalClientCommandResponse(const std::string& sessionId,
																					ExternalProtocol::CommandResponse::Type type,
																					u_int32_t messageCounter) {
	ExternalProtocol::ExternalClient externalMessage;
	ExternalProtocol::CommandResponse* commandResponse = externalMessage.mutable_commandresponse();
	commandResponse->set_sessionid(sessionId);
	commandResponse->set_type(type);
	commandResponse->set_messagecounter(messageCounter);
	return externalMessage;
}


}