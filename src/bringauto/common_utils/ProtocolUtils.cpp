#include <bringauto/common_utils/ProtocolUtils.hpp>



namespace bringauto::common_utils {

InternalProtocol::InternalServer ProtocolUtils::CreateServerMessage(const InternalProtocol::Device &device,
																	const InternalProtocol::DeviceConnectResponse_ResponseType &resType) {
	InternalProtocol::InternalServer message;
	auto response = message.mutable_deviceconnectresponse();
	response->set_responsetype(resType);
	auto device_ = response->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalServer ProtocolUtils::CreateServerMessage(const InternalProtocol::Device &device,
																	const std::string &data) {
	InternalProtocol::InternalServer message;
	auto command = message.mutable_devicecommand();
	command->set_commanddata(data);
	auto device_ = command->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalClient ProtocolUtils::CreateClientMessage(const InternalProtocol::Device &device) {
	InternalProtocol::InternalClient message;
	auto connect = message.mutable_deviceconnect();
	auto device_ = connect->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalClient ProtocolUtils::CreateClientMessage(const InternalProtocol::Device &device,
																	const std::string &data) {
	InternalProtocol::InternalClient message;
	auto status = message.mutable_devicestatus();
	status->set_statusdata(data);
	auto device_ = status->mutable_device();
	device_->CopyFrom(device);
	return message;
}

ExternalProtocol::ExternalClient
ProtocolUtils::CreateExternalClientStatus(const std::string& sessionId, ExternalProtocol::Status_DeviceState,
										  u_int32_t messageCounter, buffer statusData,
										  device_identification device, buffer errorMessage) {
	ExternalProtocol::ExternalClient externalMessage;
	ExternalProtocol::Status* status = externalMessage.mutable_status();
	auto deviceStatus = status->mutable_devicestatus();
	auto deviceMsg = deviceStatus->mutable_device();
	deviceMsg->CopyFrom(common_utils::ProtocolUtils::CreateDevice(device.module, device.device_type, device.device_role, device.device_name, device.priority));
	status->set_sessionid(sessionId);
	status->set_devicestate(ExternalProtocol::Status_DeviceState_CONNECTING);
	status->set_messagecounter(messageCounter);
	deviceStatus->set_statusdata(statusData.data, statusData.size_in_bytes);
	if (errorMessage.size_in_bytes > 0 && errorMessage.data != nullptr) {
		status->set_errormessage(errorMessage.data, errorMessage.size_in_bytes);
	}
	return externalMessage;
}

InternalProtocol::Device
ProtocolUtils::CreateDevice(int module, unsigned int type, const std::string &role, const std::string &name, unsigned int priority) {
	InternalProtocol::Device device;
	device.set_module(static_cast<InternalProtocol::Device::Module>(module));
	device.set_devicetype(type);
	device.set_devicerole(role);
	device.set_devicename(name);
	device.set_priority(priority);
	return device;
}
}