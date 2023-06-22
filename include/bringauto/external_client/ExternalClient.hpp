#pragma once


#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <InternalProtocol.pb.h>

#include <memory>
#include <vector>
#include <list>
#include "InternalProtocol.pb.h"



namespace bringauto::external_client {

class ExternalClient {
public:

	ExternalClient(std::shared_ptr <structures::GlobalContext> &context,
				   std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> &toExternalQueue);

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

    void handleCommands();

    void handleCommand(const InternalProtocol::DeviceCommand &deviceCommand);

	void sendStatus(const InternalProtocol::DeviceStatus& deviceStatus);

	std::map<unsigned int, std::reference_wrapper<connection::ExternalConnection>> externalConnectionMap_;

	std::list <connection::ExternalConnection> externalConnectionsList_;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> toExternalQueue_;

    std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> fromExternalQueue_;

    std::thread fromExternalClientThread_;

	std::shared_ptr <structures::GlobalContext> context_;
};

}
