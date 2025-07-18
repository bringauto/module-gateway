#pragma once

#include <bringauto/structures/DeviceIdentification.hpp>
#include <bringauto/modules/Buffer.hpp>

#include <InternalProtocol.pb.h>
#include <ExternalProtocol.pb.h>



namespace bringauto::common_utils {
/**
 * @brief Class of methods to create protobuf messages defined in fleet protocol and for copying protobuf data to buffers
 */
class ProtobufUtils {
public:
	ProtobufUtils() = delete;

	struct BufferNotAllocated: public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	struct BufferTooSmall: public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

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
	 * @brief Create an Internal Server Command message
	 *
	 * @param device Protobuf message Device
	 * @param command command data
	 * @return InternalProtocol::InternalServer
	 */
	static InternalProtocol::InternalServer createInternalServerCommandMessage(const InternalProtocol::Device &device,
																			   const modules::Buffer &command);

	/**
	 * @brief Create an Internal Client Status message
	 *
	 * @param device Protobuf message Device
	 * @param status status data
	 * @return InternalProtocol::InternalClient
	 */
	static InternalProtocol::InternalClient createInternalClientStatusMessage(const InternalProtocol::Device &device,
																			  const modules::Buffer &status);

	/**
	 * @brief Create a Device Status message
	 *
	 * @param deviceId device identification
	 * @param status status data
	 * @return InternalProtocol::DeviceStatus
	 */
	static InternalProtocol::DeviceStatus createDeviceStatus(const structures::DeviceIdentification &deviceId,
															 const modules::Buffer &status);

	/**
	 * @brief Create an External Client Connect message
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
																		const std::vector<structures::DeviceIdentification> &devices);

	/**
	 * @brief Create an External Client Status message
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
																	   const modules::Buffer &errorMessage = modules::Buffer {});

	/**
	 * @brief Create an External Client Command Response object
	 *
	 * @param sessionId session identification
	 * @param type command response message type
	 * @param messageCounter
	 * @return ExternalProtocol::ExternalClient
	 */
	static ExternalProtocol::ExternalClient createExternalClientCommandResponse(const std::string &sessionId,
																				ExternalProtocol::CommandResponse::Type type,
																				u_int32_t messageCounter);

	/**
	 * @brief Copy status data from DeviceStatus to a Buffer
	 * 
	 * @param status status to be copied 
	 * @param buffer buffer to copy to
	 */
	static void copyStatusToBuffer(const InternalProtocol::DeviceStatus &status, const modules::Buffer &buffer);

	/**
	 * @brief Copy command data from DeviceCommand to a Buffer
	 * 
	 * @param command command to be copied 
	 * @param buffer buffer to copy to
	 */
	static void copyCommandToBuffer(const InternalProtocol::DeviceCommand &command, const modules::Buffer &buffer);

};
}
