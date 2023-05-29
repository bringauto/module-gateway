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

InternalProtocol::Device ProtocolUtils::CreateDevice(const InternalProtocol::Device_Module &

module,
size_t type,
const std::string &role,
const std::string &name, size_t
priority) {
InternalProtocol::Device device;
device.set_module(module);
device.
set_devicetype(type);
device.
set_devicerole(role);
device.
set_devicename(name);
device.
set_priority(priority);
return
device;
}
}