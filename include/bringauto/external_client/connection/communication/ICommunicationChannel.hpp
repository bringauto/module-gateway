#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <utility>

#include <ExternalProtocol.pb.h>



namespace bringauto::external_client::connection::communication {

/**
 * @brief Generic communication interface between module gateway and external server
 *  - all communication interfaces have to inherit from this class
 */
class ICommunicationChannel {
public:
	explicit ICommunicationChannel(structures::ExternalConnectionSettings settings): settings_ {std::move( settings )} {};

	virtual ~ICommunicationChannel() = default;

	/**
	 * @brief Initialize connection with the server
	 *
	 */
	virtual void initializeConnection() = 0;

	/**
	 * @brief Send message
	 *
	 * @param message data, which are sent
	 *
	 * @return bool true if message was sent successfully, false otherwise
	 */
	virtual bool sendMessage(ExternalProtocol::ExternalClient *message) = 0;

	/**
	 * @brief Receive message
	 *
	 * @return std::shared_ptr<ExternalProtocol::ExternalServer>
	 */
	virtual std::shared_ptr<ExternalProtocol::ExternalServer> receiveMessage() = 0;

	/**
	 * @brief Stop and destroy connection
	 *
	 */
	virtual void closeConnection() = 0;

protected:
	/// Instance of the specific settings for the communication channel
	structures::ExternalConnectionSettings settings_ {};
};

}
