#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ModuleLibrary.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/external_client/connection/messages/SentMessagesHandler.hpp>
#include <bringauto/external_client/ErrorAggregator.hpp>
#include <bringauto/external_client/connection/ConnectionState.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>
#include <bringauto/structures/ReconnectQueueItem.hpp>

#include <general_error_codes.h>

#include <string>
#include <vector>
#include <thread>



namespace bringauto::external_client::connection {
/**
 * @brief Class representing connection to one external server endpoint
 */
class ExternalConnection {
public:
	ExternalConnection(const std::shared_ptr <structures::GlobalContext> &context,
					   structures::ModuleLibrary &moduleLibrary,
					   const structures::ExternalConnectionSettings &settings,
					   const std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> &commandQueue,
					   const std::shared_ptr <structures::AtomicQueue<
							   structures::ReconnectQueueItem>
	>& reconnectQueue);

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
     *
     * @param connectedDevices devices that are connected to the internal server
     * @return 0 if OK otherwise NOT_OK
	 */
	int initializeConnection(std::vector<structures::DeviceIdentification> connectedDevices);

	/**
	 * @brief Disconnect from the external server
	 * complete disconnect means it also clears aggregators
	 * otherwise it fills error aggregators with not acknowledged status messages
	 *
	 * @param completeDisconnect
	 */
	void endConnection(bool completeDisconnect);

	/**
	 * @brief Send status message to the external server
	 *
	 * @param status status message
	 * @param deviceState state of the device
	 * @param errorMessage error message
	 */
	void sendStatus(const InternalProtocol::DeviceStatus &status,
					ExternalProtocol::Status::DeviceState deviceState = ExternalProtocol::Status::DeviceState::Status_DeviceState_RUNNING,
					const buffer &errorMessage = {});

	/**
	 * @brief Check if any device is connected to the external connection
	 *
	 * @return true if yes otherwise false
	 */
	bool hasAnyDeviceConnected();

	/**
	 * @brief Force aggregation on all devices in all modules that connection service
	 * Is used before connect sequence to assure that every device has available status to be sent
	 *
	 * @return number of devices
	 */
	std::vector<structures::DeviceIdentification> forceAggregationOnAllDevices(std::vector<structures::DeviceIdentification> connectedDevices);

	/**
	 * @brief Fill error aggregator with not acknowledged status messages
	 */
	void fillErrorAggregatorWithNotAckedStatuses();

	/**
	 * @brief Fill error aggregator with not acknowledged status messages and given status message
	 *
	 * @param deviceStatus status message
	 */
	void fillErrorAggregator(const InternalProtocol::DeviceStatus &deviceStatus);

	/**
	 * @brief Get connection state
	 *
	 * @return ConnectionState
	 */
	[[nodiscard]] ConnectionState getState() const;

	/**
	 * @brief Set state to NotInitialized
	 */
	void setNotInitialized();

	/**
	 * @brief Check if module type is supported
	 *
	 * @param moduleNum module type number
	 * @return true if moudle type is supported otherwise false
	 */
	bool isModuleSupported(int moduleNum);

	std::vector <structures::DeviceIdentification> getAllConnectedDevices();

private:

	/**
	 * @brief Generate and set the session id
	 */
	void setSessionId();

	[[nodiscard]] u_int32_t getNextStatusCounter();

	[[nodiscard]] static u_int32_t getCommandCounter(const ExternalProtocol::Command &command);

	int connectMessageHandle(const std::vector <structures::DeviceIdentification> &devices);

	/**
	 * @brief Takes care of second etape of connect sequence - for all devices send their last status
	 * @param devices
	 */
	int statusMessageHandle(const std::vector <structures::DeviceIdentification> &devices);

	int commandMessageHandle(const std::vector <structures::DeviceIdentification> &devices);

	/**
	 * @brief Check if command is in order and send commandResponse
	 *
	 * @param commandMessage
	 * @return 0 if OK
	 * @return -1 if command is out of order
	 * @return -2 if command has incorrect session ID
	 */
	int handleCommand(const ExternalProtocol::Command &commandMessage);

	/**
	 * @brief This loop is started after successful connect sequence in own thread.
	 * Loop is receiving messages from external server and processes them.
	 */
	void receivingHandlerLoop();

	std::atomic<bool> stopReceiving { false };

	const int KEY_LENGHT { 8 };

	u_int32_t clientMessageCounter_ { 0 };

	u_int32_t serverMessageCounter_ { 0 };

	std::string sessionId_ {};

	std::unique_ptr <communication::ICommunicationChannel> communicationChannel_ {};

	std::jthread listeningThread;

	/**
	 * State of the car
	 * - thread safe
	 */
	std::atomic <ConnectionState> state_ { ConnectionState::NOT_INITIALIZED };

	std::shared_ptr <structures::GlobalContext> context_;

	structures::ModuleLibrary &moduleLibrary_;

	const structures::ExternalConnectionSettings &settings_;

	std::unique_ptr <messages::SentMessagesHandler> sentMessagesHandler_;

	std::map<unsigned int, ErrorAggregator> errorAggregators;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> commandQueue_;

	std::shared_ptr <structures::AtomicQueue<structures::ReconnectQueueItem>>
	reconnectQueue_;

	std::string carId_ {};

	std::string vehicleName_ {};

	std::string company_ {};
};

}