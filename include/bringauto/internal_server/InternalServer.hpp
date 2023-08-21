#pragma once


#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/structures/Connection.hpp>
#include <bringauto/structures/ConnectionContext.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>

#include <memory>
#include <thread>



namespace bringauto::internal_server {
/**
 * Server implements internal protocol. Serves as link between Module handler and Internal client..
 * It accepts connections from multiple Internal clients.
 * Receives messages on these connections. The message needs to begin with 4 bytes header.
 * Header's format is 32 bit unsigned int with little endian endianity.
 * Header represents size of the remaining part of the message.
 * Messaged is send thru queue to ModuleHandler, and when answer is given resends it to Internal client.
 */
class InternalServer {

public:
	/**
	 * @brief Constructs Internal Server.
	 * @param settings shares context with Internal Server
	 * @param fromInternalQueue queue for sending data from Server to Module Handler
	 * @param toInternalQueue queue for sending data from Module Handler to Server
	 */
	InternalServer(const std::shared_ptr <structures::GlobalContext> &context,
				   const std::shared_ptr <structures::AtomicQueue<structures::InternalClientMessage>> &fromInternalQueue,
				   const std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalServer>> &toInternalQueue)
			: context_ { context }, acceptor_(context->ioContext), fromInternalQueue_ { fromInternalQueue },
			  toInternalQueue_ { toInternalQueue } {}

	~InternalServer() = default;

	/**
	 * Starts the server.
	 * - Async acceptor task i added to the io_context,
	 * - Starts Thread that listens to data coming from ModuleHandler
	 */
	void start();

	/**
	 * Stop the server.
	 * - Stops Acceptor.
	 * - Notifies all running threads to end.
	 * Must be called at the end of the Server instance lifetime.
	 * After the stop() the start() can be called to reinitialized the Server instance.
	 */
	void stop();

private:
	/**
	 * Asynchronously accepts new connections.
	 * Once a connection is accepted the async_receive task is added to the io_context.
	 */
	void addAsyncAccept();

	/**
	 * @brief Asynchronously receives data.
	 * @param connection connection that data are being sent thru
	 */
	void addAsyncReceive(const std::shared_ptr <structures::Connection> &connection);

	/**
	 * @brief Handles received data and connection. If error occurs connection is closed and cleaned.
	 * @param connection handled connection
	 * @param error possible error code
	 * @param bytesTransferred size of received data
	 */
	void asyncReceiveHandler(const std::shared_ptr <structures::Connection> &connection,
							 const boost::system::error_code &error, std::size_t bytesTransferred);

	/**
	 * @brief Processes buffer data and once message is complete calls handleMessage(...)
	 * @param connection conneection with context holding received and processed data
	 * @param bytesTransferred  size of received data
	 * @return true if data and whole message is correct in context to fleet protocol
	 */
	bool processBufferData(const std::shared_ptr <structures::Connection> &connection,
						   std::size_t bytesTransferred);

	/**
	 * Parses received data into Protobuf message, checks validity.
	 * If all is correct calls handleStatus(...) or handleConnect(...),
	 * if true is returned waits for timeout or notification: response was resent to client / server is being stopped.
	 * @param connection connection with data to be parsed into message
	 * @return true if everything was successful and method was notified that response was resent to client
	 */
	bool handleMessage(const std::shared_ptr <structures::Connection> &connection);

	/**
	 * @brief Checks if status is valid, if it is sends message to Module Handler.
	 * @param connection connection with information about validity
	 * @param client message to be checked and sent
	 * @return true if status is valid.
	 */
	bool handleStatus(const std::shared_ptr <structures::Connection> &connection,
					  const InternalProtocol::InternalClient &client);

	/**
	 * @brief Checks for existance of this device and possibly its priority and calls matching method.
	 * @param connection connection holding data
	 * @param client message to be checked
	 * @return true if connect is valid.
	 */
	bool handleConnection(const std::shared_ptr <structures::Connection> &connection,
						  const InternalProtocol::InternalClient &client);

	/**
	 * @brief Inserts connection into map of all active connections, sends message to module Handler.
	 * @param connection connection to be inserted into map
	 * @param connect message to be sent
	 * @param deviceId unique device identification
	 */
	void connectNewDevice(const std::shared_ptr <structures::Connection> &connection,
						  const InternalProtocol::InternalClient &connect,
						  const std::shared_ptr <structures::DeviceIdentification> &deviceId);

	/**
	 * @brief Sends response to InternalClient, that device is already connected and with higher priority.
	 * @param connection connection response will be sent thru
	 * @param connect message containing data for response message
	 */
	void respondWithHigherPriorityConnected(const std::shared_ptr <structures::Connection> &connection,
											const InternalProtocol::InternalClient &connect,
											const std::shared_ptr <structures::DeviceIdentification> &deviceId);

	/**
	 * @brief Sends response to InternalClient, that device is already connected and with same priority.
	 * @param connection connection response will be sent thru
	 * @param connect message containing data for response message
	 */
	void respondWithAlreadyConnected(const std::shared_ptr <structures::Connection> &connection,
									 const InternalProtocol::InternalClient &connect,
									 const std::shared_ptr <structures::DeviceIdentification> &deviceId);

	/**
	 * Ends all operations of previous connection using same device,
	 * closes its socket, then replaces it in map with new connection.
	 * Afterwards sends new connection message to Module Handler.
	 * @param connection new connection to replace the old one
	 * @param connect message to be sent
	 * @param deviceId unique device identification
	 */
	void changeConnection(const std::shared_ptr <structures::Connection> &connection,
						  const InternalProtocol::InternalClient &connect,
						  const std::shared_ptr <structures::DeviceIdentification> &deviceId);

	/**
	 * @brief Writes messages to Internal client.
	 * @param connection connection message will be sent thru
	 * @param message message to be sent
	 * @return true if writes are successful
	 */
	bool sendResponse(const std::shared_ptr <structures::Connection> &connection,
					  const InternalProtocol::InternalServer &message);

	/**
	 * @brief Removes Connection from the map of active connections and clean up/closes its socket
	 * @param connection connection to be removed
	 */
	void removeConnFromMap(const std::shared_ptr <structures::Connection> &connection);

	/**
	 * Periodicly checks for new messages received from module handler thru queue.
	 * If message is received calls validatesResponse(...).
	 * Runs until stop() is called.
	 */
	void listenToQueue();

	/**
	 * Validates if message belongs to any active connection, if it does resends the message to InternalClient,
	 * then notifies waiting connection.
	 * @param message message to be validated
	 */
	void validateResponse(const InternalProtocol::InternalServer &message);

	/**
	 * @brief Searches for connection in connectedDevices_ vector with deviceId, which is "same" as given deviceId.
	 * @param deviceId that will be searched for in the vector
	 * @return connection in connectedDevices_ vector
	 */
	std::shared_ptr <structures::Connection> findConnection(structures::DeviceIdentification *deviceId);

	std::shared_ptr <structures::GlobalContext> context_;
	boost::asio::ip::tcp::acceptor acceptor_;
	std::shared_ptr <structures::AtomicQueue<structures::InternalClientMessage>> fromInternalQueue_;
	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalServer>> toInternalQueue_;

	std::mutex serverMutex_;
	std::vector <std::shared_ptr<structures::Connection>> connectedDevices_;

	std::jthread listeningThread;

};

}