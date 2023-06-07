#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <utility>

namespace bringauto::external_client::connection::communication {

/**
 * @brief Generic communication interface, all communication interfaces have to inherit from this class
 */
class ICommunicationChannel {
public:
	explicit ICommunicationChannel(structures::ExternalConnectionSettings settings);

	virtual ~ICommunicationChannel() = default;

	virtual int initializeConnection() = 0;

	virtual int sendMessage() = 0;

	// TODO getCommand, or just add Commands to CommandQueue from listening thread

	virtual void closeConnection() = 0;

protected:
	structures::ExternalConnectionSettings settings_;
};

}