#pragma once

#include <bringauto/settings/Settings.hpp>

namespace bringauto::external_client::connection::communication {

/**
 * @brief Generic communication interface, all communication interfaces have to inherit from this class
 */
class ICommunicationChannel {
public:
	ICommunicationChannel(settings::Settings settings);

	virtual ~ICommunicationChannel() = default;


};

}