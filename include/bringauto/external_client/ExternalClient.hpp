#pragma once


#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/structures/GlobalContext.hpp>

#include <vector>



namespace bringauto::external_client {

class ExternalClient {
public:
	/**
	 * @brief Initialize connections, error aggregators ?Queue?
	 */
	ExternalClient(structures::GlobalContext context);
private:
	std::map<int, connection::ExternalConnection> connectionMap_;
	// TODO timers map<connection, timer>
};

}
