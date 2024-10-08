#include <bringauto/common_utils/ProtobufUtils.hpp>

#include <fleet_protocol/common_headers/general_error_codes.h>

#include <sstream>



namespace bringauto::common_utils {

InternalProtocol::InternalServer
ProtobufUtils::createInternalServerConnectResponseMessage(const InternalProtocol::Device &device,
														  const InternalProtocol::DeviceConnectResponse_ResponseType &resType) {
	InternalProtocol::InternalServer message {};
	auto response = message.mutable_deviceconnectresponse();
	response->set_responsetype(resType);
	auto device_ = response->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalServer
ProtobufUtils::createInternalServerCommandMessage(const InternalProtocol::Device &device,
												  const modules::Buffer &command) {
	InternalProtocol::InternalServer message {};
	auto deviceCommand = message.mutable_devicecommand();
	if (command.isAllocated()) {
		deviceCommand->set_commanddata(command.getStructBuffer().data, command.getStructBuffer().size_in_bytes);
	}
	auto device_ = deviceCommand->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalClient
ProtobufUtils::createInternalClientStatusMessage(const InternalProtocol::Device &device,
												 const modules::Buffer &status) {
	InternalProtocol::InternalClient message {};
	auto deviceStatus = message.mutable_devicestatus();
	if (status.isAllocated()) {
		deviceStatus->set_statusdata(status.getStructBuffer().data, status.getStructBuffer().size_in_bytes);
	}
	auto device_ = deviceStatus->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::DeviceStatus ProtobufUtils::createDeviceStatus(const structures::DeviceIdentification &deviceId,
																 const modules::Buffer &status) {
	InternalProtocol::DeviceStatus deviceStatus {};
	deviceStatus.mutable_device()->CopyFrom(deviceId.convertToIPDevice());
	if (status.isAllocated()) {
		deviceStatus.set_statusdata(status.getStructBuffer().data, status.getStructBuffer().size_in_bytes);
	}
	return deviceStatus;
}

ExternalProtocol::ExternalClient ProtobufUtils::createExternalClientConnect(const std::string &sessionId,
																			const std::string &company,
																			const std::string &vehicleName,
																			const std::vector<structures::DeviceIdentification> &devices) {
	ExternalProtocol::ExternalClient externalMessage {};
	auto connectMessage = externalMessage.mutable_connect();

	connectMessage->set_sessionid(sessionId);
	connectMessage->set_company(company);
	connectMessage->set_vehiclename(vehicleName);

	for(const auto &tmpDevice: devices) {
		auto devicePtr = connectMessage->add_devices();
		devicePtr->CopyFrom(tmpDevice.convertToIPDevice());
	}

	return externalMessage;
}

ExternalProtocol::ExternalClient ProtobufUtils::createExternalClientStatus(const std::string &sessionId,
																		   ExternalProtocol::Status_DeviceState deviceState,
																		   u_int32_t messageCounter,
																		   const InternalProtocol::DeviceStatus &deviceStatus,
																		   const modules::Buffer &errorMessage) {
	ExternalProtocol::ExternalClient externalMessage {};
	ExternalProtocol::Status *status = externalMessage.mutable_status();
	status->mutable_devicestatus()->CopyFrom(deviceStatus);
	status->set_sessionid(sessionId);
	status->set_devicestate(deviceState);
	status->set_messagecounter(messageCounter);
	if(errorMessage.isAllocated()) {
		status->set_errormessage(errorMessage.getStructBuffer().data, errorMessage.getStructBuffer().size_in_bytes);
	}
	return externalMessage;
}

ExternalProtocol::ExternalClient ProtobufUtils::createExternalClientCommandResponse(const std::string &sessionId,
																					ExternalProtocol::CommandResponse::Type type,
																					u_int32_t messageCounter) {
	ExternalProtocol::ExternalClient externalMessage {};
	ExternalProtocol::CommandResponse *commandResponse = externalMessage.mutable_commandresponse();
	commandResponse->set_sessionid(sessionId);
	commandResponse->set_type(type);
	commandResponse->set_messagecounter(messageCounter);
	return externalMessage;
}

void ProtobufUtils::copyStatusToBuffer(const InternalProtocol::DeviceStatus &status, modules::Buffer &buffer) {
	if (!buffer.isAllocated()) {
		throw BufferNotAllocated { "Buffer is not allocated" };
	}
	if (status.statusdata().size() > buffer.getStructBuffer().size_in_bytes) {
		throw BufferTooSmall { "Buffer does not have enough space allocated for status" };
	}
	std::memcpy(buffer.getStructBuffer().data, status.statusdata().c_str(), status.statusdata().size());
}

void ProtobufUtils::copyCommandToBuffer(const InternalProtocol::DeviceCommand &command, modules::Buffer &buffer) {
	if (!buffer.isAllocated()) {
		throw BufferNotAllocated { "Buffer is not allocated" };
	}
	if (command.commanddata().size() > buffer.getStructBuffer().size_in_bytes) {
		throw BufferTooSmall { "Buffer does not have enough space allocated for command" };
	}
	std::memcpy(buffer.getStructBuffer().data, command.commanddata().c_str(), command.commanddata().size());
}

}
