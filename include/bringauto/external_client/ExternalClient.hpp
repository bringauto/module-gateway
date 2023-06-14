#pragma once


#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <InternalProtocol.pb.h>

#include <memory>
#include <vector>



namespace bringauto::external_client {

class ExternalClient {
public:

	ExternalClient(std::shared_ptr <structures::GlobalContext> &context,
				   std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> &toExternalQueue)
			: context_ { context }, toExternalQueue_ { toExternalQueue } {};

	/**
	 * @brief Initialize connections, error aggregators ?Queue?
	 */
	void run();

	/**
	 * @brief
	 */
	void destroy();

private:

	void initConnections();

	void handleAggregatedMessages();

	void sendMessage(InternalProtocol::InternalClient &message);

	std::map<unsigned int, std::reference_wrapper<connection::ExternalConnection>> connectionMap_;

	std::vector <connection::ExternalConnection> externalConnections_;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> toExternalQueue_;

	std::shared_ptr <structures::GlobalContext> context_;

	// TODO timers map<connection, timer>
};

}
