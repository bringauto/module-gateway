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
	ExternalConnection( const std::shared_ptr<structures::GlobalContext>& context,
						const structures::ExternalConnectionSettings& settings,
						const std::string& company,
						const std::string& vehicleName,
						const std::shared_ptr<structures::AtomicQueue<InternalProtocol::DeviceCommand>>& commandQueue);
	/**
	 * @brief Handles all etapes of connect sequence. If connect sequence is successful,
     * infinite receive loop is started in new thread.
	 */
	void initializeConnection();

	void endConnection();

    void sendStatus(const InternalProtocol::DeviceStatus &status, ExternalProtocol::Status::DeviceState deviceState = ExternalProtocol::Status::DeviceState::Status_DeviceState_RUNNING);

	bool hasAnyDeviceConnected();
private:

    void setSessionId();

	void fillErrorAggregator(InternalProtocol::DeviceStatus);

	u_int32_t getNextStatusCounter();

	u_int32_t getCommandCounter(ExternalProtocol::Command command);

	void connectMessageHandle(std::vector<device_identification> devices);

	/**
	 * @brief Takes care of second etape of connect sequence - for all devices send their last status
	 * @param devices
	 */
	void statusMessageHandle(std::vector<device_identification> devices);

	void commandMessageHandle(std::vector<device_identification> devices);

	void handleCommand(ExternalProtocol::Command commandMessage);

	/**
	 * @brief This loop is started after successful connect sequence in own thread.
     * Loop is receiving messages from external server and processes them.
	 */
	void receivingHandlerLoop();

    const int KEY_LENGHT = 8;

	u_int32_t clientMessageCounter_ { 0 };

	u_int32_t serverMessageCounter_ { 0 };

	std::string sessionId_ {};

	std::unique_ptr<communication::ICommunicationChannel> communicationChannel_ {};
	bool isConnected { false };

	std::shared_ptr <structures::GlobalContext> context_;

	structures::ExternalConnectionSettings &settings_;

	messages::SentMessagesHandler sentMessagesHandler_;
	std::map<int, ErrorAggregator> errorAggregators;

	std::shared_ptr<structures::AtomicQueue<InternalProtocol::DeviceCommand>> commandQueue_;

	std::vector<int> modules_; // TODO change to map aggregators?

	std::string carId_ {}; // TODO not needed

	std::string vehicleName_ {};

	std::string company_ {};
};

}