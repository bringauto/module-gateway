#pragma once

#include <functional>



namespace bringauto::external_client::connection {
class ExternalConnection;
}

namespace bringauto::structures {

struct ReconnectQueueItem {
public:
	explicit ReconnectQueueItem(
			const std::reference_wrapper<external_client::connection::ExternalConnection> &connection,
			bool reconnect): connection_ { connection }, reconnect_ { reconnect } {}

	std::reference_wrapper<external_client::connection::ExternalConnection> connection_;

	bool reconnect_;

};

}