#pragma once

#include <bringauto/structures/ConnectionContext.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>
#include <boost/asio.hpp>

#include <condition_variable>
#include <string>



namespace bringauto::structures {
/**
 * @brief Connection between Client and Server. (Currently only used inside InternalServer class privately)
 */
struct Connection {

	/**
	 * @brief Constructs connection, adds socket into io_context.
	 * @param io_context_ context shared across Gateway
	 */
	Connection(boost::asio::io_context &io_context_): socket(io_context_) {}

	/**
	 * @brief socket endpoint in communication between server and client
	 */
	boost::asio::ip::tcp::socket socket;
	/**
	 * @brief identification of connected device
	 */
	std::shared_ptr <DeviceIdentification> deviceId;
	/**
	 * @brief Context for connection
	 */
	bringauto::structures::ConnectionContext connContext;
	/**
	 * @brief condition variable used for wait - after a message was send from server to module handler.
	 * and notify - when response to that message was received in server and resent to internal client
	 */
	std::condition_variable conditionVariable;
	/**
	 * @brief mutex used alongside conditionVariable
	 */
	std::mutex connectionMutex;
	/**
	 * @brief variable used to decide if connection can start reading again, after sending message to Module Handler
	 */
	bool ready { false };
};

}

