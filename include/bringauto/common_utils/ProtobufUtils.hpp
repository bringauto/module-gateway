#pragma once

#include <bringauto/structures/DeviceIdentification.hpp>
#include <InternalProtocol.pb.h>
#include <ExternalProtocol.pb.h>
#include <device_management.h>
#include <memory_management.h>



namespace bringauto::common_utils {
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
	static InternalProtocol::InternalServer CreateInternalServerConnectResponseMessage(const InternalProtocol::Device &device,
																					   const InternalProtocol::DeviceConnectResponse_ResponseType &resType);

	static InternalProtocol::InternalServer CreateInternalServerCommandMessage(const InternalProtocol::Device &device,
																			   const buffer &command);

	static InternalProtocol::InternalClient CreateInternalClientStatusMessage(const InternalProtocol::Device &device,
																			  const buffer &status);

	static InternalProtocol::DeviceStatus CreateDeviceStatus(const InternalProtocol::Device &device,
															 const buffer &statusData);

	static InternalProtocol::DeviceStatus CreateDeviceStatus(const device_identification &device,
															 const buffer &statusData);


	static InternalProtocol::Device CreateDevice(int module,
												 unsigned int type,
												 const std::string &role,
												 const std::string &name,
												 unsigned int priority);

	static InternalProtocol::Device CreateDevice(const device_identification &device);

	static InternalProtocol::Device CreateDevice(const structures::DeviceIdentification &device);

	static device_identification ParseDevice(const InternalProtocol::Device &device);

	static ExternalProtocol::ExternalClient CreateExternalClientConnect(const std::string &sessionId,
																		const std::string &company,
																		const std::string &vehicleName,
																		const std::vector<device_identification> &devices);

	static ExternalProtocol::ExternalClient	CreateExternalClientConnect(const std::string &sessionId,
																	    const std::string &company,
																	    const std::string &vehicleName,
																		const std::vector<structures::DeviceIdentification> &devices);

	static ExternalProtocol::ExternalClient	CreateExternalClientStatus(const std::string &sessionId,
																	   ExternalProtocol::Status_DeviceState deviceState,
																	   u_int32_t messageCounter,
																	   const InternalProtocol::DeviceStatus &deviceStatus,
																	   const buffer &errorMessage);

	static ExternalProtocol::ExternalClient	CreateExternalClientCommandResponse(const std::string &sessionId,
																			    ExternalProtocol::CommandResponse::Type type,
																				u_int32_t messageCounter);

	static buffer ProtobufToBuffer(const google::protobuf::Message &protobufMessage);

};
}