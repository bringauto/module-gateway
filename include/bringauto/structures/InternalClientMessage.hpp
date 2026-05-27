#pragma once

#include <bringauto/structures/DeviceIdentification.hpp>

#include <InternalProtocol.pb.h>



namespace bringauto::structures {

/**
 * @brief Class for storing information about internal client message and additional information
 */
class InternalClientMessage {

public:
	explicit InternalClientMessage(const DeviceIdentification &deviceId):
		disconnect_ { true },
		deviceId_ { deviceId }
	{}

	/**
	 * @brief Create a command-forward event. Used by ExternalClient to wake up ModuleHandler
	 * for immediate command dispatch when the module requested forward_command_on_receive.
	 *
	 * @param deviceId device that has a pending command to forward
	 * @return InternalClientMessage tagged as command-forward
	 */
	static InternalClientMessage makeCommandForward(const DeviceIdentification &deviceId);

	explicit InternalClientMessage(bool disconnect, const InternalProtocol::InternalClient &message):
		message_ { message },
		disconnect_ { disconnect }
	{}

	InternalClientMessage(InternalClientMessage&&) noexcept = default;

	InternalClientMessage(const InternalClientMessage& copy) = default;

	InternalClientMessage& operator=(InternalClientMessage&&) noexcept = default;

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
	 * @brief Returns true if this message is a command-forward event (no protobuf payload).
	 */
	[[nodiscard]] bool isCommandForward() const noexcept;

	/**
	 * @brief Get device identification struct
	 *
	 * @return const DeviceIdentification&
	 */
	const DeviceIdentification &getDeviceId() const;

private:
	InternalClientMessage(const DeviceIdentification &deviceId, bool commandForward)
		: disconnect_ { false }, commandForward_ { commandForward }, deviceId_ { deviceId }
	{}

	/// Internal client message
	InternalProtocol::InternalClient message_ {};
	/// True if device is disconnected otherwise false
	bool disconnect_;
	/// True if this is a command-forward event (no protobuf payload)
	bool commandForward_ { false };
	/// Device identification struct
	DeviceIdentification deviceId_ {};
};

}
