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

#include <string>
#include <vector>
#include <thread>



namespace bringauto::external_client::connection {
/**
 * @brief Class representing a connection to one external server endpoint
 */
class ExternalConnection {
public:
	ExternalConnection(const std::shared_ptr <structures::GlobalContext> &context,
					   structures::ModuleLibrary &moduleLibrary,
					   const structures::ExternalConnectionSettings &settings,
					   const std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> &commandQueue,
					   const std::shared_ptr <structures::AtomicQueue<structures::ReconnectQueueItem>>& reconnectQueue);

	/**
	 * @brief Initialize the external connection
	 * it has to be called after the constructor
	 *
	 * @param communicationChannel communication channel to the external server
	 */
	void init(const std::shared_ptr <communication::ICommunicationChannel> &communicationChannel);

	/**
	 * @brief Handles all stages of the connect sequence. If the connect sequence is successful,
	 * an infinite receive loop is started in a new thread.
	 *
	 * @param connectedDevices devices that are connected to the internal server
	 * @return OK if successful, otherwise NOT_OK
	 */
	int initializeConnection(const std::vector<structures::DeviceIdentification>& connectedDevices);

	/**
	 * @brief Disconnect from the external server
	 * complete disconnect means it also clears aggregators
	 * otherwise it fills error aggregators with not acknowledged status messages
	 *
	 * @param completeDisconnect
	 */
	void deinitializeConnection(bool completeDisconnect);

	/**
	 * @brief Send a status message to the external server
	 *
	 * @param status status message
	 * @param deviceState state of the device
	 * @param errorMessage error message
	 */
	void sendStatus(const InternalProtocol::DeviceStatus &status,
					ExternalProtocol::Status::DeviceState deviceState = ExternalProtocol::Status::DeviceState::Status_DeviceState_RUNNING,
					const modules::Buffer& errorMessage = modules::Buffer {});

	/**
	 * @brief Force aggregation on all devices in all modules that the connection services.
	 * Is used before the connect sequence to assure that every device has an available status to be sent
	 *
	 * @param connectedDevices
	 * @return number of devices
	 */
	std::vector<structures::DeviceIdentification> forceAggregationOnAllDevices(const std::vector<structures::DeviceIdentification> &connectedDevices);

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
	 * @return true if module type is supported otherwise false
	 */
	bool isModuleSupported(int moduleNum) const;

	std::vector <structures::DeviceIdentification> getAllConnectedDevices() const;

private:

	/**
	 * @brief Generate and set the session id
	 */
	void generateSessionId();

	[[nodiscard]] u_int32_t getNextStatusCounter();

	[[nodiscard]] static u_int32_t getCommandCounter(const ExternalProtocol::Command &command);

	int connectMessageHandle(const std::vector <structures::DeviceIdentification> &devices);

	/**
	 * @brief Takes care of second stage of the connect sequence - send the last status of all devices
	 * @param devices
	 */
	int statusMessageHandle(const std::vector <structures::DeviceIdentification> &devices);

	int commandMessageHandle(const std::vector <structures::DeviceIdentification> &devices);

	/**
	 * @brief Check if command is in order and send commandResponse
	 *
	 * @param commandMessage
	 * @return OK if successful
	 * @return COMMAND_INVALID if command is out of order or has incorrect session ID
	 * @return NOT_OK otherwise
	 */
	int handleCommand(const ExternalProtocol::Command &commandMessage);

	/**
	 * @brief This loop is started after successful connect sequence in own thread.
	 * Loop is receiving messages from external server and processes them.
	 */
	void receivingHandlerLoop();
	/// Indication if receiving loop should be stopped
	std::atomic<bool> stopReceiving { false };
	/// Length of the key used for identification
	const int KEY_LENGTH { 8 };
	/// Counter for sent messages
	u_int32_t clientMessageCounter_ { 0 };
	/// Counter for received messages
	u_int32_t serverMessageCounter_ { 0 };
	/// ID of the current external connection session, changes with every connect sequence
	std::string sessionId_ {};
	/// Communication channel to the external server
	std::shared_ptr <communication::ICommunicationChannel> communicationChannel_ {};
	/// Thread for receiving loop
	std::jthread listeningThread {};

	/// State of the external connection - thread safe
	std::atomic <ConnectionState> state_ { ConnectionState::NOT_INITIALIZED };

	std::shared_ptr <structures::GlobalContext> context_ {};

	structures::ModuleLibrary &moduleLibrary_;

	const structures::ExternalConnectionSettings &settings_ {};
	/// Class handling sent messages - timers, not acknowledged statuses etc.
	std::unique_ptr <messages::SentMessagesHandler> sentMessagesHandler_ {};
	/// @brief Map of error aggregators, key is module number
	std::unordered_map<unsigned int, ErrorAggregator> errorAggregators_ {};
	/// Queue of commands received from external server, commands are processed by aggregator
	std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> commandQueue_ {};

	std::shared_ptr <structures::AtomicQueue<structures::ReconnectQueueItem>>
	reconnectQueue_ {};
	/// Name of the vehicle
	std::string vehicleName_ {};
	/// Name of the company
	std::string company_ {};
};

}
