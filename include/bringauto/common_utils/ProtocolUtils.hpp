#pragma once

#include <InternalProtocol.pb.h>



namespace bringauto::common_utils {
/**
 * @brief Class of methods to create protobuf messages defined in fleet protocol
 */
class ProtocolUtils {
public:
	ProtocolUtils() = delete;

	/**
	 * @brief Creates InternalServer message with the "one of" being DeviceConnectResponse.
	 * @param device Protobuf message Device
	 * @param resType Response type
	 * @return Protobuf message InternalServer
	 */
	static InternalProtocol::InternalServer CreateServerMessage(const InternalProtocol::Device &device,
														  const InternalProtocol::DeviceConnectResponse_ResponseType &resType);
	static InternalProtocol::InternalServer CreateServerMessage(const InternalProtocol::Device &device,
														  const std::string &data);
	static InternalProtocol::InternalClient CreateClientMessage(const InternalProtocol::Device &device);
	static InternalProtocol::InternalClient CreateClientMessage(const InternalProtocol::Device &device,
																const std::string &data);

	static InternalProtocol::Device CreateDevice(const InternalProtocol::Device_Module &module, size_t type,
												 const std::string &role, const std::string &name, size_t priority);
};
}