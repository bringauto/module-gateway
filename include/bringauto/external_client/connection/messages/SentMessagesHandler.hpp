#pragma once

#include <bringauto/external_client/connection/messages/NotAckedStatus.hpp>

#include <ExternalProtocol.pb.h>

namespace bringauto::external_client::connection::messages {

class SentMessagesHandler {
public:
	SentMessagesHandler() {};

	/**
	 * @brief call this method for each sent status - will add status as not acknowledged
	 * @param status
	 */
	void addNotAckedStatus(ExternalProtocol::ExternalClient status);

	/**
	 * @brief called when a status response arrives
	 * @param statusResponse
	 */
	void acknowledgeStatus(ExternalProtocol::ExternalServer statusResponse);

	bool allStatusesAcked();

	void clearAll();

	void clearAllTimers(); // TODO private?

	void addDeviceAsConnected();

	void deleteConnectedDevice();

	bool isDeviceConnected();

	bool isAnyDeviceConnected();
private:
	/**
	 * @brief returns message counter of status
	 * @param status
	 * @return counter
	 */
	u_int32_t getStatusCounter(ExternalProtocol::ExternalClient status);

	/**
	 * @brief returns message counter of status_response
	 * @param status
	 * @return counter
	 */
	u_int32_t getStatusResponseCounter(ExternalProtocol::ExternalServer statusResponse);

	std::vector<NotAckedStatus> notAckedStatuses_;
	std::vector<InternalProtocol::Device> connectedDevices_;

};

}
