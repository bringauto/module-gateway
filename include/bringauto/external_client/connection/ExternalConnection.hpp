#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <string>
#include <vector>



namespace bringauto::external_client::connection {
/**
 * @brief Class representing connection to one external server endpoint
 */
class ExternalConnection {
public:
	ExternalConnection(const structures::ExternalConnectionSettings &settings): ip_{settings.serverIp}, port_{settings.port} {};

    void send();
private:
	int messageCounter_ {};
	std::string sessionId_ {};
	std::unique_ptr<communication::ICommunicationChannel> communicationChannel_;
	// Settings - TODO save only context and extract this
	std::string ip_; // TODO merge into boost buffer?
	u_int16_t port_;
	// std::vector<int> modules_; // TODO change to map aggregators?

	std::string carId_ {};
	std::string vehicleName_ {};
	std::string company_ {};
};

}