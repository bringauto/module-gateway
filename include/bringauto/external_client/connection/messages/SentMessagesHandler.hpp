#pragma once

#include <bringauto/external_client/connection/messages/NotAckedStatus.hpp>
#include <bringauto/structures/GlobalContext.hpp>
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
	void addDeviceAsConnected(const std::string &device);

	/**
	 * @brief Delete connected device
	 *
	 * @param device connected device id
	 */
	void deleteConnectedDevice(const std::string &device);

	/**
	 * @brief Check if device is connected
	 *
	 * @param device connected device id
	 * @return true if given device is connected otherwise false
	 */
	bool isDeviceConnected(const std::string &device);

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
	[[nodiscard]] u_int32_t getStatusCounter(const ExternalProtocol::Status &status);

	/**
	 * @brief returns message counter of status_response
	 * @param status
	 * @return counter
	 */
	[[nodiscard]] u_int32_t getStatusResponseCounter(const ExternalProtocol::StatusResponse &statusResponse);

	std::vector <std::shared_ptr<NotAckedStatus>> notAckedStatuses_;

	std::vector <std::string> connectedDevices_;

	std::shared_ptr <structures::GlobalContext> context_;

	std::atomic<bool> responseHandled_ { false };

	std::mutex responseHandledMutex_;

	std::function<void()> endConnectionFunc_;
};

}
