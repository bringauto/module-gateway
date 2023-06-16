#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/external_client/connection/messages/SentMessagesHandler.hpp>
#include <bringauto/external_client/ErrorAggregator.hpp>

#include <string>
#include <vector>



namespace bringauto::external_client::connection {
/**
 * @brief Class representing connection to one external server endpoint
 */
class ExternalConnection {
public:
	explicit ExternalConnection(std::shared_ptr<structures::GlobalContext> &context,
								const structures::ExternalConnectionSettings &settings,
								std::string company,
								std::string vehicleName,
								const std::shared_ptr<structures::AtomicQueue<InternalProtocol::DeviceCommand>>& commandQueue);
	/**
	 * @brief Handles all etapes of connect sequence. If connect sequence is successful,
     * infinite receive loop is started in new thread.
	 */
	void initializeConnection();
	void endConnection();
    void sendStatus(InternalProtocol::DeviceStatus, ExternalProtocol::Status::DeviceState = ExternalProtocol::Status::DeviceState::Status_DeviceState_RUNNING);

	bool hasAnyDeviceConnected();
private:
	void fillErrorAggregator(InternalProtocol::DeviceStatus);

	u_int32_t getNextStatusCounter();
	// TODO u_int32 getCommandCounter(command) ??

	void connectMessageHandle(std::vector<device_identification> devices);

	void statusMessageHandle(std::vector<device_identification> devices);

	void commandMessageHandle(std::vector<device_identification> devices);

	void handleCommand(ExternalProtocol::Command commandMessage);

	/**
	 * @brief This loop is started after successful connect sequence in own thread.
     * Loop is receiving messages from external server and processes them.
	 */
	void receivingHandlerLoop();


	u_int32_t clientMessageCounter_ {};
	u_int32_t serverMessageCounter_ {};
	std::string sessionId_ {};
	std::unique_ptr<communication::ICommunicationChannel> communicationChannel_ {};
	bool isConnected { false };

	std::shared_ptr <structures::GlobalContext> context;
	structures::ExternalConnectionSettings settings;

	std::map<int, ErrorAggregator> errorAggregators;
	std::shared_ptr<structures::AtomicQueue<InternalProtocol::DeviceCommand>> commandQueue_;

	messages::SentMessagesHandler sentMessagesHandler_;

	// std::vector<int> modules_; // TODO change to map aggregators?

	std::string carId_ {}; // TODO not needed
	std::string vehicleName_ {};
	std::string company_ {};
};

}