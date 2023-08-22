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
	static InternalProtocol::InternalServer
	createInternalServerConnectResponseMessage(const InternalProtocol::Device &device,
											   const InternalProtocol::DeviceConnectResponse_ResponseType &resType);

	/**
	 * @brief Create a Internal Server Command message
	 *
	 * @param device Protobuf message Device
	 * @param command command data
	 * @return InternalProtocol::InternalServer
	 */
	static InternalProtocol::InternalServer createInternalServerCommandMessage(const InternalProtocol::Device &device,
																			   const buffer &command);

	/**
	 * @brief Create a Internal Client Status message
	 *
	 * @param device Protobuf message Device
	 * @param status status data
	 * @return InternalProtocol::InternalClient
	 */
	static InternalProtocol::InternalClient createInternalClientStatusMessage(const InternalProtocol::Device &device,
																			  const buffer &status);

	/**
	 * @brief Create a Device Status message
	 *
	 * @param deviceId device identification
	 * @param status status data
	 * @return InternalProtocol::DeviceStatus
	 */
	static InternalProtocol::DeviceStatus createDeviceStatus(const device_identification &deviceId,
															 const buffer &status);

	/**
	 * @brief Create a Device protobuf object
	 *
	 * @param module module number
	 * @param type type of the device
	 * @param role role of the device
	 * @param name name of the device
	 * @param priority device priority
	 * @return InternalProtocol::Device
	 */
	static InternalProtocol::Device createDevice(int module,
												 unsigned int type,
												 const std::string &role,
												 const std::string &name,
												 unsigned int priority);

	/**
	 * @brief Create a Device protobuf object
	 *
	 * @param device device identification
	 * @return InternalProtocol::Device
	 */
	static InternalProtocol::Device createDevice(const device_identification &device);

	/**
	 * @brief Create a Device protobuf object
	 *
	 * @param device DeviceIdentification
	 * @return InternalProtocol::Device
	 */
	static InternalProtocol::Device createDevice(const structures::DeviceIdentification &device);

	/**
	 * @brief Create device identification from protobuf Device
	 *
	 * @param device protobuf
	 * @return device_identification
	 */
	static device_identification parseDevice(const InternalProtocol::Device &device);

	/**
	 * @brief Create a External Client Connect message
	 *
	 * @param sessionId session identification
	 * @param company name of the company
	 * @param vehicleName name of the vehicle
	 * @param devices vector of device that want to connect
	 * @return ExternalProtocol::ExternalClient
	 */
	static ExternalProtocol::ExternalClient createExternalClientConnect(const std::string &sessionId,
																		const std::string &company,
																		const std::string &vehicleName,
																		const std::vector <structures::DeviceIdentification> &devices);

	/**
	 * @brief Create a External Client Status message
	 *
	 * @param sessionId session identification
	 * @param deviceState state of the device
	 * @param messageCounter
	 * @param deviceStatus protobuf message
	 * @param errorMessage error buffer
	 * @return ExternalProtocol::ExternalClient
	 */
	static ExternalProtocol::ExternalClient createExternalClientStatus(const std::string &sessionId,
																	   ExternalProtocol::Status_DeviceState deviceState,
																	   u_int32_t messageCounter,
																	   const InternalProtocol::DeviceStatus &deviceStatus,
																	   const buffer &errorMessage);

	/**
	 * @brief Create a External Client Command Response object
	 *
	 * @param sessionId session identification
	 * @param type command response message type
	 * @param messageCounter
	 * @return ExternalProtocol::ExternalClient
	 */
	static ExternalProtocol::ExternalClient createExternalClientCommandResponse(const std::string &sessionId,
																				ExternalProtocol::CommandResponse::Type type,
																				u_int32_t messageCounter);

	static std::string getId(const ::device_identification &device);

};
}