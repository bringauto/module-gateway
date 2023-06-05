#pragma once

#include <string>
#include <vector>


namespace bringauto::external_client::connection {
/**
 * @brief Class representing connection to one external server endpoint
 */
class ExternalConnection {
private:
	int messageCounter_ {};
	std::string sessionId_ {};
// TODO generic connection
	// Settings - TODO save only context and extract this
	std::string ip_; // TODO merge into boost buffer?
	u_int16_t port_;
	std::vector<int> modules_; // TODO change to map aggregators?

	std::string carId_ {};
	std::string vehicleName_ {};
	std::string company_ {};
};

}