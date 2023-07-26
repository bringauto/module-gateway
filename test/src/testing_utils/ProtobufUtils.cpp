#include <testing_utils/ProtobufUtils.hpp>

namespace testing_utils {

InternalProtocol::InternalServer ProtobufUtils::CreateServerMessage(const InternalProtocol::Device &device,
																	const InternalProtocol::DeviceConnectResponse_ResponseType &resType) {
	InternalProtocol::InternalServer message;
	auto response = message.mutable_deviceconnectresponse();
	response->set_responsetype(resType);
	auto device_ = response->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalServer ProtobufUtils::CreateServerMessage(const InternalProtocol::Device &device,
																	const std::string &data) {
	InternalProtocol::InternalServer message;
	auto command = message.mutable_devicecommand();
	command->set_commanddata(data);
	auto device_ = command->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalClient ProtobufUtils::CreateClientMessage(const InternalProtocol::Device &device) {
	InternalProtocol::InternalClient message;
	auto connect = message.mutable_deviceconnect();
	auto device_ = connect->mutable_device();
	device_->CopyFrom(device);
	return message;
}

InternalProtocol::InternalClient ProtobufUtils::CreateClientMessage(const InternalProtocol::Device &device,
																	const std::string &data) {
	InternalProtocol::InternalClient message;
	auto status = message.mutable_devicestatus();
	status->set_statusdata(data);
	auto device_ = status->mutable_device();
	device_->CopyFrom(device);
	return message;
}

}