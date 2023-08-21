#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <ExternalProtocol.pb.h>

#include <utility>



namespace bringauto::external_client::connection::communication {

/**
 * @brief Generic communication interface between module gateway and external server
 *  - all communication interfaces have to inherit from this class
 */
class ICommunicationChannel {
public:
	explicit ICommunicationChannel(const structures::ExternalConnectionSettings &settings): settings_ { settings } {};

	virtual ~ICommunicationChannel() = default;

	/**
	 * @brief Set the basic properties to the communication object
	 *
	 * @param company name of the company
	 * @param vehicleName name of the vehicle
	 */
	virtual void setProperties(const std::string &company, const std::string &vehicleName) = 0;

	virtual int initializeConnection() = 0;

	/**
	 * @brief Send message
	 *
	 * @param message data, which are sent
	 */
	virtual void sendMessage(ExternalProtocol::ExternalClient *message) = 0;

	/**
	 * @brief Receives message
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
	structures::ExternalConnectionSettings settings_;
};

}