#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <ExternalProtocol.pb.h>

#include <utility>

namespace bringauto::external_client::connection::communication {

/**
 * @brief Generic communication interface, all communication interfaces have to inherit from this class
 */
class ICommunicationChannel {
public:
	explicit ICommunicationChannel(const structures::ExternalConnectionSettings &settings) : settings_{settings}{};

	virtual ~ICommunicationChannel() = default;

    virtual void setProperties(const std::string& company, const std::string& vehicleName) = 0;

	virtual int initializeConnection() = 0;

	virtual int sendMessage(ExternalProtocol::ExternalClient *message) = 0;

	virtual std::shared_ptr<ExternalProtocol::ExternalServer> receiveMessage() = 0;

	virtual void closeConnection() = 0;

protected:
	structures::ExternalConnectionSettings settings_;
};

}