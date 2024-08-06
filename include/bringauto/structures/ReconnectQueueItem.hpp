#pragma once

#include <functional>



namespace bringauto::external_client::connection {
class ExternalConnection;
}

namespace bringauto::structures {

/**
 * @brief Item in reconnect queue in external client
 */
struct ReconnectQueueItem {
public:
	explicit ReconnectQueueItem(
			const std::reference_wrapper<external_client::connection::ExternalConnection> &connection,
			bool reconnect): connection_ { connection }, reconnect { reconnect } {}

	std::reference_wrapper<external_client::connection::ExternalConnection> connection_;

	/**
	 * @brief If true, connection will be reconnected
	 */
	bool reconnect;
};

}
