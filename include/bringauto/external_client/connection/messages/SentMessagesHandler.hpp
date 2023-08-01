#pragma once

#include <bringauto/external_client/connection/messages/NotAckedStatus.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <utility>

#include <ExternalProtocol.pb.h>



namespace bringauto::external_client::connection::messages {

class SentMessagesHandler {
public:
	explicit SentMessagesHandler(const std::shared_ptr <structures::GlobalContext> &context,
								 const std::function<void()> endConnectionFunc);

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

	[[nodiscard]] const std::vector <std::shared_ptr<NotAckedStatus>> &
	getNotAckedStatus() const { return notAckedStatuses_; }

	/**
	 * @brief Return true if all statuses were acknowledged
	 * @return
	 */
	[[nodiscard]] bool allStatusesAcked() const { return notAckedStatuses_.empty(); };

	void clearAll();

	void addDeviceAsConnected(const InternalProtocol::Device &device);

	void deleteConnectedDevice(const InternalProtocol::Device &device);

	bool isDeviceConnected(InternalProtocol::Device device);

	[[nodiscard]] bool isAnyDeviceConnected() { return connectedDevices_.empty(); };

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

	std::vector <InternalProtocol::Device> connectedDevices_;

	std::shared_ptr <structures::GlobalContext> context_;

	std::atomic<bool> responseHandled_ = false;

	std::mutex responseHandledMutex_;

	std::function<void()> endConnectionFunc_;
};

}
