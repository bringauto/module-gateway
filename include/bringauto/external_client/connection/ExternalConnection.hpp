#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/external_client/connection/messages/SentMessagesHandler.hpp>
#include <bringauto/external_client/ErrorAggregator.hpp>
#include <bringauto/external_client/connection/ConnectionState.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>

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
					   const std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> &commandQueue,
					   const std::shared_ptr<structures::AtomicQueue<std::reference_wrapper<connection::ExternalConnection>>>& reconnectQueue);

    /**
     * @brief Initialize external connection
     * it has to be called after constructor
     *
     * @param company name of the company
     * @param vehicleName name of the vehicle
     */
    void init(const std::string &company, const std::string &vehicleName);

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

	/**
	 * @brief Force aggregation on all devices in all modules that connection service
	 * Is used before connect sequence to assure that every device has available status to be sent
	 *
	 * @return number of devices
	 */
	int forceAggregationOnAllDevices();

	void fillErrorAggregator();

	void fillErrorAggregator(const InternalProtocol::DeviceStatus& deviceStatus);

	[[nodiscard]] ConnectionState getState() const { return state_.load(); }

	bool isModuleSupported(int moduleNum);
private:
	void setSessionId();

	u_int32_t getNextStatusCounter();

	static u_int32_t getCommandCounter(const ExternalProtocol::Command& command);

	int connectMessageHandle(const std::vector<structures::DeviceIdentification> &devices);

	/**
	 * @brief Takes care of second etape of connect sequence - for all devices send their last status
	 * @param devices
	 */
	int statusMessageHandle(const std::vector<structures::DeviceIdentification> &devices);

	int commandMessageHandle(const std::vector<structures::DeviceIdentification> &devices);

	/**
	 * @brief Check if command is in order and send commandResponse
	 * @param commandMessage
	 * @return 0 if OK
	 * @return -1 if command is out of order
	 * @return -2 if command has incorrect session ID
	 */
	int handleCommand(const ExternalProtocol::Command& commandMessage);

	/**
	 * @brief This loop is started after successful connect sequence in own thread.
     * Loop is receiving messages from external server and processes them.
	 */
	void receivingHandlerLoop();

	std::vector<structures::DeviceIdentification> getAllConnectedDevices();

	std::atomic<bool> stopReceiving { false };

	const int KEY_LENGHT = 8;

	u_int32_t clientMessageCounter_ { 0 };

	u_int32_t serverMessageCounter_ { 0 };

	std::string sessionId_ {};

	std::unique_ptr <communication::ICommunicationChannel> communicationChannel_ {};

	std::thread listeningThread;

	/**
	 * State of the car
	 * - thread safe
	 */
	std::atomic<ConnectionState> state_ { ConnectionState::NOT_INITIALIZED };

	std::shared_ptr <structures::GlobalContext> context_;

	const structures::ExternalConnectionSettings &settings_;

	std::unique_ptr<messages::SentMessagesHandler> sentMessagesHandler_;

	std::map<unsigned int, ErrorAggregator> errorAggregators;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> commandQueue_;

	std::shared_ptr<structures::AtomicQueue<std::reference_wrapper<connection::ExternalConnection>>> reconnectQueue_;

	std::string carId_ {};

	std::string vehicleName_ {};

	std::string company_ {};
};

}