#pragma once

#include <fleet_protocol/common_headers/device_management.h>
#include <InternalProtocol.pb.h>



namespace bringauto::structures {

/**
 * @brief Class for storing information about module handler message and additional information
 */
class ModuleHandlerMessage {

public:
	explicit ModuleHandlerMessage(const device_identification &deviceId):
		disconnect_ { true },
		deviceId_ { deviceId }
	{}

	explicit ModuleHandlerMessage(bool disconnect, const InternalProtocol::InternalServer &message):
		message_ { message },
		disconnect_ { disconnect }
	{}

	/**
	 * @brief Get internal client message
	 *
	 * @return const InternalProtocol::InternalServer&
	 */
	const InternalProtocol::InternalServer &getMessage() const;

	/**
	 * @brief Get disconnected
	 *
	 * @return true if device is to be disconnected otherwise false
	 */
	bool disconnected() const;

	/**
	 * @brief Get device identification struct
	 *
	 * @return const device_identification&
	 */
	const device_identification &getDeviceId() const;

private:
	/// Internal server message
	InternalProtocol::InternalServer message_ {};
	/// True if device is to be disconnected otherwise false
	bool disconnect_;
	/// Device identification struct
	device_identification deviceId_ {};
};

}
