#pragma once


#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <InternalProtocol.pb.h>

#include <memory>
#include <vector>
#include <list>



namespace bringauto::external_client {

class ExternalClient {
public:

	ExternalClient(std::shared_ptr <structures::GlobalContext> &context,
				   std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> &toExternalQueue);

	/**
	 * @brief Initialize connections, error aggregators
	 */
	void run();

	/**
	 * @brief Stops connections and joins thread for command handling
	 */
	void destroy();

private:

	void initConnections();

	void startExternalConnectSequence(connection::ExternalConnection& connection);

	void handleAggregatedMessages();

    void handleCommands();

    void handleCommand(const InternalProtocol::DeviceCommand &deviceCommand);

	void sendStatus(const InternalProtocol::DeviceStatus &deviceStatus);

	bool insideConnectSequence_ = false;

	std::map<unsigned int, std::reference_wrapper<connection::ExternalConnection>> externalConnectionMap_;

	std::list <connection::ExternalConnection> externalConnectionsList_;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> toExternalQueue_;

    std::shared_ptr <structures::AtomicQueue<InternalProtocol::DeviceCommand>> fromExternalQueue_;

	std::shared_ptr <structures::AtomicQueue<std::reference_wrapper<connection::ExternalConnection>>> reconnectQueue_;

    std::jthread fromExternalClientThread_;

	std::shared_ptr <structures::GlobalContext> context_;
};

}
