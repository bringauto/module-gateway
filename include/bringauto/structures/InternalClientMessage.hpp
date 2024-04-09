#pragma once

#include <fleet_protocol/common_headers/device_management.h>
#include <InternalProtocol.pb.h>



namespace bringauto::structures {

/**
 * @brief Class for storing information about internal client message and additional information
 */
class InternalClientMessage {

public:
	explicit InternalClientMessage(const device_identification &deviceId):
		disconnect_ { true },
		deviceId_ { deviceId }
	{}

	explicit InternalClientMessage(bool disconnect, const InternalProtocol::InternalClient &message):
		message_ { message },
		disconnect_ { disconnect }
	{}

	/**
	 * @brief Get internal client message
	 *
	 * @return const InternalProtocol::InternalClient&
	 */
	const InternalProtocol::InternalClient &getMessage() const;

	/**
	 * @brief Get disconnected
	 *
	 * @return true if device is disconnected otherwise false
	 */
	bool disconnected() const;

	/**
	 * @brief Get device identification struct
	 *
	 * @return const device_identification&
	 */
	const device_identification &getDeviceId() const;

private:
	/// Internal client message
	InternalProtocol::InternalClient message_ {};
	/// True if device is disconnected otherwise false
	bool disconnect_;
	/// Device identification struct
	device_identification deviceId_ {};
};

}
