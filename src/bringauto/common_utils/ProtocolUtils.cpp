#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/common_utils/MemoryUtils.hpp>

#include <sstream>

#include <general_error_codes.h>



namespace bringauto::common_utils {

InternalProtocol::InternalServer
ProtobufUtils::createInternalServerConnectResponseMessage(const InternalProtocol::Device &device,
														  const InternalProtocol::DeviceConnectResponse_ResponseType &resType) {
	InternalProtocol::InternalServer message;
	auto response = message.mutable_deviceconnectresponse();
	response->set_responsetype(resType);
	auto device_ = response->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalServer
ProtobufUtils::createInternalServerCommandMessage(const InternalProtocol::Device &device,
												  const buffer &command) {
	InternalProtocol::InternalServer message;
	auto deviceCommand = message.mutable_devicecommand();
	deviceCommand->set_commanddata(command.data, command.size_in_bytes);
	auto device_ = deviceCommand->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalClient
ProtobufUtils::createInternalClientStatusMessage(const InternalProtocol::Device &device,
												 const buffer &status) {
	InternalProtocol::InternalClient message;
	auto deviceStatus = message.mutable_devicestatus();
	deviceStatus->set_statusdata(status.data, status.size_in_bytes);
	auto device_ = deviceStatus->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::DeviceStatus ProtobufUtils::createDeviceStatus(const device_identification &deviceId,
																 const buffer &status) {
    InternalProtocol::DeviceStatus deviceStatus;
	deviceStatus.mutable_device()->CopyFrom(createDevice(deviceId));
	deviceStatus.set_statusdata(status.data, status.size_in_bytes);
	return deviceStatus;
}

device_identification ProtobufUtils::parseDevice(const InternalProtocol::Device &device) {
    struct buffer deviceRoleBuff{};
    MemoryUtils::initBuffer(deviceRoleBuff, device.devicerole());

	struct buffer deviceNameBuff{};
    MemoryUtils::initBuffer(deviceNameBuff, device.devicename());

	return ::device_identification { .module = device.module(),
			.device_type = device.devicetype(),
			.device_role = deviceRoleBuff,
			.device_name = deviceNameBuff,
			.priority = device.priority() };
}

InternalProtocol::Device ProtobufUtils::createDevice(const device_identification &device) {
    std::string device_role{static_cast<char *>(device.device_role.data), device.device_role.size_in_bytes};
    std::string device_name{static_cast<char *>(device.device_name.data), device.device_name.size_in_bytes};
	return createDevice(device.module, device.device_type, device_role, device_name, device.priority);
}

InternalProtocol::Device ProtobufUtils::createDevice(const structures::DeviceIdentification &device) {
	return createDevice(device.getModule(), device.getDeviceType(), device.getDeviceRole(), device.getDeviceName(),
						device.getPriority());
}

InternalProtocol::Device
ProtobufUtils::createDevice(int module, unsigned int type, const std::string &role, const std::string &name,
							unsigned int priority) {
	InternalProtocol::Device device;
	device.set_module(static_cast<InternalProtocol::Device::Module>(module));
	device.set_devicetype(type);
	device.set_devicerole(role);
	device.set_devicename(name);
	device.set_priority(priority);
	return device;
}

ExternalProtocol::ExternalClient ProtobufUtils::createExternalClientConnect(const std::string &sessionId,
																			const std::string &company,
																			const std::string &vehicleName,
																			const std::vector <structures::DeviceIdentification> &devices) {
	ExternalProtocol::ExternalClient externalMessage;
	auto connectMessage = externalMessage.mutable_connect();

	connectMessage->set_sessionid(sessionId);
	connectMessage->set_company(company);
	connectMessage->set_vehiclename(vehicleName);

	for(const auto &tmpDevice: devices) {
		auto devicePtr = connectMessage->add_devices();
		devicePtr->CopyFrom(createDevice(tmpDevice));
	}

	return externalMessage;
}

ExternalProtocol::ExternalClient ProtobufUtils::createExternalClientStatus(const std::string &sessionId,
																		   ExternalProtocol::Status_DeviceState deviceState,
																		   u_int32_t messageCounter,
																		   const InternalProtocol::DeviceStatus &deviceStatus,
																		   const buffer &errorMessage) {
	ExternalProtocol::ExternalClient externalMessage;
	ExternalProtocol::Status *status = externalMessage.mutable_status();
	status->mutable_devicestatus()->CopyFrom(deviceStatus);
	status->set_sessionid(sessionId);
	status->set_devicestate(deviceState);
	status->set_messagecounter(messageCounter);
	if(errorMessage.size_in_bytes > 0 && errorMessage.data != nullptr) {
		status->set_errormessage(errorMessage.data, errorMessage.size_in_bytes);
	}
	return externalMessage;
}

ExternalProtocol::ExternalClient ProtobufUtils::createExternalClientCommandResponse(const std::string &sessionId,
																					ExternalProtocol::CommandResponse::Type type,
																					u_int32_t messageCounter) {
	ExternalProtocol::ExternalClient externalMessage;
	ExternalProtocol::CommandResponse *commandResponse = externalMessage.mutable_commandresponse();
	commandResponse->set_sessionid(sessionId);
	commandResponse->set_type(type);
	commandResponse->set_messagecounter(messageCounter);
	return externalMessage;
}

std::string ProtobufUtils::getId(const ::device_identification &device) {
	std::stringstream ss;
	ss << device.module << "/" << device.device_type << "/" << std::string{static_cast<char *>(device.device_role.data), device.device_role.size_in_bytes} << "/"
	   << std::string{static_cast<char *>(device.device_name.data), device.device_name.size_in_bytes}; // TODO we need to be able to get priority
	return ss.str();
}

}