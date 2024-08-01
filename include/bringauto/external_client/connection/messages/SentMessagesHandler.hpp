#pragma once

#include <bringauto/external_client/connection/messages/NotAckedStatus.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>
#include <utility>

#include <ExternalProtocol.pb.h>



namespace bringauto::external_client::connection::messages {

class SentMessagesHandler {
public:
	explicit SentMessagesHandler(const std::shared_ptr <structures::GlobalContext> &context,
								 const std::function<void()> &endConnectionFunc);

	/**
	 * @brief call this method for each sent status - will add status as not acknowledged
	 * @param status
	 */
	void addNotAckedStatus(const ExternalProtocol::Status &status);

	/**
	 * @brief called when a status response arrives
	 * @param statusResponse
	 */
	int acknowledgeStatus(const ExternalProtocol::StatusResponse &statusResponse);

	/**
	 * @brief Get not acknowledged statuses messages
	 *
	 * @return const std::vector <std::shared_ptr<NotAckedStatus>>&
	 */
	[[nodiscard]] const std::vector <std::shared_ptr<NotAckedStatus>> &getNotAckedStatuses() const;

	/**
	 * @brief Return true if all statuses were acknowledged
	 * @return
	 */
	[[nodiscard]] bool allStatusesAcked() const;

	/**
	 * @brief Clear all timers and erase them
	 */
	void clearAll();

	/**
	 * @brief Add connected device
	 *
	 * @param device connected device id
	 */
	void addDeviceAsConnected(const structures::DeviceIdentification& device);

	/**
	 * @brief Delete connected device
	 *
	 * @param device connected device id
	 */
	void deleteConnectedDevice(const structures::DeviceIdentification& device);

	/**
	 * @brief Check if device is connected
	 *
	 * @param device connected device id
	 * @return true if given device is connected otherwise false
	 */
	bool isDeviceConnected(const structures::DeviceIdentification &device);

	/**
	 * @brief Check if any device is connected
	 *
	 * @return true if any device is connected otherwise false
	 */
	[[nodiscard]] bool isAnyDeviceConnected() const;

	/**
	 * @brief Clear all timers
	 */
	void clearAllTimers();

private:

	/**
	 * @brief returns message counter of status
	 * @param status
	 * @return counter
	 */
	[[nodiscard]] static u_int32_t getStatusCounter(const ExternalProtocol::Status &status);

	/**
	 * @brief returns message counter of status_response
	 * @param status
	 * @return counter
	 */
	[[nodiscard]] static u_int32_t getStatusResponseCounter(const ExternalProtocol::StatusResponse &statusResponse);

	/// Vector of statuses not acknowledged by the external server
	std::vector <std::shared_ptr<NotAckedStatus>> notAckedStatuses_ {};
	/// Vector of connected devices, the value is device id - @see ProtobufUtils::getId()
	std::vector <structures::DeviceIdentification> connectedDevices_ {};
	/// Global context of module gateway
	std::shared_ptr <structures::GlobalContext> context_ {};
	/// Callback called by timer when status does not get response, registered by constructor
	std::function<void()> endConnectionFunc_ {};

	std::atomic<bool> responseHandled_ { false };

	std::mutex responseHandledMutex_ {};
	std::mutex ackMutex_ {};
};

}
