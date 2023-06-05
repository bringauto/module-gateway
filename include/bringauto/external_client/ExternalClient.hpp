#pragma once


#include <vector>
#include <bringauto/external_client/connection/ExternalConnection.hpp>

namespace bringauto::external_client {

class ExternalClient {
private:
	std::vector<connection::ExternalConnection> connectionList;
	// TODO timers map<connection, timer>
};

}
