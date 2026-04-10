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
	 * @return std::unique_ptr<ExternalProtocol::ExternalServer>
	 */
	virtual std::unique_ptr<ExternalProtocol::ExternalServer> receiveMessage() = 0;

	/**
	 * @brief Stop and destroy connection
	 *
	 */
	virtual void closeConnection() = 0;

	/**
	 * @brief Unblock any pending receiveMessage() call immediately.
	 * Used during shutdown to abort an in-progress connect attempt without
	 * the full receive_message_timeout delay.
	 */
	virtual void cancelReceive() = 0;

	/**
	 * @brief Check whether the external server sent a disconnect notification.
	 *
	 * Returns true and clears the internal flag if a disconnect notification has
	 * been received since the last call. Returns false otherwise.
	 * Used by ExternalConnection to trigger an immediate reconnect instead of
	 * waiting for the status_response_timeout to expire.
	 */
	virtual bool consumeServerDisconnectNotification() = 0;

protected:
	/// Instance of the specific settings for the communication channel
	structures::ExternalConnectionSettings settings_ {};
};

}
