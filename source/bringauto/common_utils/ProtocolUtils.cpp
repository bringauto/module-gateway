#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/common_utils/MemoryUtils.hpp>

#include <fleet_protocol/common_headers/general_error_codes.h>

#include <sstream>



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

InternalProtocol::DeviceStatus ProtobufUtils::createDeviceStatus(const structures::DeviceIdentification &deviceId,
																 const buffer &status) {
	InternalProtocol::DeviceStatus deviceStatus;
	deviceStatus.mutable_device()->CopyFrom(deviceId.convertToIPDevice());
	deviceStatus.set_statusdata(status.data, status.size_in_bytes);
	return deviceStatus;
}

ExternalProtocol::ExternalClient ProtobufUtils::createExternalClientConnect(const std::string &sessionId,
																			const std::string &company,
																			const std::string &vehicleName,
																			const std::vector<structures::DeviceIdentification> &devices) {
	ExternalProtocol::ExternalClient externalMessage;
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

}