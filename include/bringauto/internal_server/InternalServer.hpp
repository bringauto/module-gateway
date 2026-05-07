#pragma once

#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/structures/Connection.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>
#include <bringauto/structures/ModuleHandlerMessage.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>

#include <list>
#include <thread>



namespace bringauto::internal_server {
/**
 * Server implements internal protocol. Serves as link between Module handler and Internal client.
 * It accepts connections from multiple Internal clients.
 * Receives messages on these connections. The message needs to begin with 4 bytes header.
 * Header's format is 32 bit unsigned int with little endian endianness.
 * Header represents size of the remaining part of the message.
 * Message is sent through queue to ModuleHandler, and when answer is given resent to Internal client.
 */
class InternalServer {

public:
	/**
	 * @brief Constructs Internal Server.
	 * @param context shares context with Internal Server
	 * @param fromInternalQueue queue for sending data from Server to Module Handler
	 * @param toInternalQueue queue for sending data from Module Handler to Server
	 */
	InternalServer(structures::GlobalContext &context,
				   structures::AtomicQueue<structures::InternalClientMessage>& fromInternalQueue,
				   structures::AtomicQueue<structures::ModuleHandlerMessage>& toInternalQueue)
			: context_ { context }, acceptor_(context.ioContext), fromInternalQueue_ { fromInternalQueue },
			  toInternalQueue_ { toInternalQueue } {}

	~InternalServer();

	/**
	 * Starts the server.
	 * - Async acceptor task added to the io_context,
	 * - Starts Thread that listens to data coming from ModuleHandler
	 */
	void run();

private:
	/**
	 * Asynchronously accepts new connections.
	 * Once a connection is accepted the async_receive task is added to the io_context.
	 */
	void addAsyncAccept();

	/**
	 * @brief Asynchronously receives data.
	 * @param connection connection that data are being sent through
	 */
	void addAsyncReceive(structures::Connection &connection);

	/**
	 * @brief Handles received data and connection. If error occurs connection is closed and cleaned.
	 * @param connection handled connection
	 * @param error possible error code
	 * @param bytesTransferred size of received data
	 */
	void asyncReceiveHandler(structures::Connection &connection,
							 const boost::system::error_code &error, std::size_t bytesTransferred);

	/**
	 * @brief Processes buffer data and once message is complete calls handleMessage(...)
	 * @param connection connection with context holding received and processed data
	 * @param bytesTransferred  size of received data
	 * @param bufferOffset offset of buffer where data starts
	 * @return true if data and whole message is correct in context to fleet protocol
	 */
	bool processBufferData(structures::Connection &connection,
						   std::size_t bytesTransferred, std::size_t bufferOffset = 0);

	/**
	 * Parses received data into Protobuf message, checks validity.
	 * If all is correct calls handleStatus(...) or handleConnect(...),
	 * if true is returned waits for timeout or notification: response was resent to client / server is being stopped.
	 * @param connection connection with data to be parsed into message
	 * @return true if everything was successful and method was notified that response was resent to client
	 */
	bool handleMessage(structures::Connection &connection);

	/**
	 * @brief Checks if status is valid. If it is, the message is sent to Module Handler.
	 * @param connection connection with information about validity
	 * @param client message to be checked and sent
	 * @return true if status is valid.
	 */
	bool handleStatus(structures::Connection &connection,
					  const InternalProtocol::InternalClient &client) const;

	/**
	 * @brief Checks for existence of this device and possibly its priority and calls matching method.
	 * @param connection connection holding data
	 * @param client message to be checked
	 * @return true if connect is valid.
	 */
	bool handleConnection(structures::Connection &connection,
						  const InternalProtocol::InternalClient &client);

	/**
	 * @brief Disconnects device and removes it from the active connections.
	 * @param deviceId unique device identification
	 */
	void handleDisconnect(const structures::DeviceIdentification& deviceId);

	/**
	 * @brief Activates a connection already in the list: assigns its deviceId and notifies Module Handler.
	 * @param connection connection to activate
	 * @param connect message to be sent
	 * @param deviceId unique device identification
	 */
	void connectNewDevice(structures::Connection &connection,
						  const InternalProtocol::InternalClient &connect,
						  const structures::DeviceIdentification &deviceId);

	/**
	 * @brief Sends response to InternalClient, that device is already connected and with higher priority.
	 * @param connection connection response will be sent through
	 * @param connect message containing data for response message
	 * @param deviceId unique device identification
	 */
	void respondWithHigherPriorityConnected(structures::Connection &connection,
											const InternalProtocol::InternalClient &connect,
											const structures::DeviceIdentification &deviceId);

	/**
	 * @brief Sends response to InternalClient, that device is already connected and with same priority.
	 * @param connection connection response will be sent through
	 * @param connect message containing data for response message
	 * @param deviceId unique device identification
	 */
	void respondWithAlreadyConnected(structures::Connection &connection,
									 const InternalProtocol::InternalClient &connect,
									 const structures::DeviceIdentification &deviceId);

	/**
	 * Marks the existing connection for the device as pending removal (closes its socket),
	 * then activates the new connection.
	 * Afterward sends new connection message to Module Handler.
	 * @param newConnection new connection to activate
	 * @param connect message to be sent
	 * @param deviceId unique device identification
	 */
	void changeConnection(structures::Connection &newConnection,
						  const InternalProtocol::InternalClient &connect,
						  const structures::DeviceIdentification &deviceId);

	/**
	 * @brief Writes messages to Internal client.
	 * @param connection connection message will be sent through
	 * @param message message to be sent
	 * @return true if writes are successful
	 */
	bool sendResponse(structures::Connection &connection,
					  const InternalProtocol::InternalServer &message);

	/**
	 * @brief Closes the connection's socket and erases it from connectedDevices_.
	 *        Sends a disconnect notification to Module Handler unless markedForRemoval is set.
	 * @param connection connection to be removed (caller must hold serverMutex_)
	 */
	void removeConnFromMap(structures::Connection &connection);

	/**
	 * Periodically checks for new messages received from module handler through queue.
	 * If message is received calls validatesResponse(...).
	 * Runs until stop() is called.
	 */
	void listenToQueue();

	/**
	 * Validates if message belongs to any active connection. If it does, the message is resent to InternalClient,
	 * then the waiting connection is notified.
	 * @param message message to be validated
	 */
	void validateResponse(const InternalProtocol::InternalServer &message);

	/**
	 * @brief Searches for a non-removed connection in connectedDevices_ with a deviceId "same" as the given one.
	 * @param deviceId that will be searched for in the list
	 * @return pointer to connection in connectedDevices_, or nullptr if not found (caller must hold serverMutex_)
	 */
	structures::Connection* findConnection(const structures::DeviceIdentification &deviceId);

	structures::GlobalContext& context_;
	boost::asio::ip::tcp::acceptor acceptor_;
	/// Queue for messages from Module Handler to Internal Client
	structures::AtomicQueue<structures::InternalClientMessage>& fromInternalQueue_;
	/// Queue for messages from Internal Client to Module Handler
	structures::AtomicQueue<structures::ModuleHandlerMessage>& toInternalQueue_;

	std::mutex serverMutex_ {};
	/// All live connections (pending accept + identified devices); stable addresses, never relocated
	std::list<structures::Connection> connectedDevices_;
	/// Thread that listens to queue for messages from Module Handler
	std::jthread listeningThread {};
};

}
