#pragma once

#include <InternalProtocol.pb.h>



namespace testing_utils {
/**
 * @brief Class of methods to create protobuf messages defined in fleet protocol
 */
class ProtobufUtils {
public:
	ProtobufUtils() = delete;

	/**
	 * @brief Creates InternalServer message with the "one of" being DeviceConnectResponse.
	 * @param device Protobuf message Device
	 * @param resType Response type
	 * @return Protobuf message InternalServer
	 */
	static InternalProtocol::InternalServer CreateServerMessage(
		const InternalProtocol::Device &device,
		const InternalProtocol::DeviceConnectResponse_ResponseType &resType
	);
	static InternalProtocol::InternalServer CreateServerMessage(
		const InternalProtocol::Device &device,
		const std::string &data
	);
	static InternalProtocol::InternalClient CreateClientMessage(const InternalProtocol::Device &device);
	static InternalProtocol::InternalClient CreateClientMessage(
		const InternalProtocol::Device &device,
		const std::string &data
	);
};
}
