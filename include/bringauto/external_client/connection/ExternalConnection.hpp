#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/external_client/connection/messages/SentMessagesHandler.hpp>
#include <bringauto/external_client/ErrorAggregator.hpp>

#include <general_error_codes.h>

#include <string>
#include <vector>



namespace bringauto::external_client::connection {
/**
 * @brief Class representing connection to one external server endpoint
 */
class ExternalConnection {
public:
	ExternalConnection(const std::shared_ptr <structures::GlobalContext> &context,
					   const structures::ExternalConnectionSettings &settings,
					   const std::string &company,
					   const std::string &vehicleName,
					   const std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> &commandQueue);

	/**
	 * @brief Handles all etapes of connect sequence. If connect sequence is successful,
     * infinite receive loop is started in new thread.
	 */
	int initializeConnection();

	void endConnection(bool completeDisconnect);

	void sendStatus(const InternalProtocol::DeviceStatus &status,
					ExternalProtocol::Status::DeviceState deviceState = ExternalProtocol::Status::DeviceState::Status_DeviceState_RUNNING,
					const buffer& errorMessage = {});

	bool hasAnyDeviceConnected();

	void fillErrorAggregator();
	void fillErrorAggregator(InternalProtocol::DeviceStatus deviceStatus);

	bool getIsConnected() const { return isConnected; }

private:
	void setSessionId();

	u_int32_t getNextStatusCounter();

	static u_int32_t getCommandCounter(const ExternalProtocol::Command& command);

	int connectMessageHandle(const std::vector <device_identification> &devices);

	/**
	 * @brief Takes care of second etape of connect sequence - for all devices send their last status
	 * @param devices
	 */
	int statusMessageHandle(const std::vector <device_identification> &devices);

	int commandMessageHandle(std::vector <device_identification> devices);

	/**
	 * @brief Check if command is in order and send commandResponse
	 * @param commandMessage
	 * @return 0 if OK
	 * @return -1 if command is out of order
	 * @return -2 if command has incorrect session ID
	 */
	int handleCommand(ExternalProtocol::Command commandMessage);

	/**
	 * @brief This loop is started after successful connect sequence in own thread.
     * Loop is receiving messages from external server and processes them.
	 */
	void receivingHandlerLoop();

	std::atomic<bool> stopReceiving { false };

	const int KEY_LENGHT = 8;

	u_int32_t clientMessageCounter_ { 0 };

	u_int32_t serverMessageCounter_ { 0 };

	std::string sessionId_ {};

	std::unique_ptr <communication::ICommunicationChannel> communicationChannel_ {};

	std::thread listeningThread;
	bool isConnected { false };

	std::shared_ptr <structures::GlobalContext> context_;

	const structures::ExternalConnectionSettings &settings_;

	messages::SentMessagesHandler sentMessagesHandler_;
	std::map<int, ErrorAggregator> errorAggregators;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> commandQueue_;

	std::vector<int> modules_; // TODO change to map aggregators?

	std::string carId_ {}; // TODO not needed

	std::string vehicleName_ {};

	std::string company_ {};
};

}