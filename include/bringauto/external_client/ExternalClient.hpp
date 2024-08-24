#pragma once

#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ModuleLibrary.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>
#include <bringauto/structures/ReconnectQueueItem.hpp>

#include <InternalProtocol.pb.h>

#include <memory>
#include <vector>
#include <list>



namespace bringauto::external_client {

class ExternalClient {
public:

	ExternalClient(std::shared_ptr<structures::GlobalContext> &context,
				   structures::ModuleLibrary &moduleLibrary,
				   std::shared_ptr<structures::AtomicQueue<structures::InternalClientMessage>> &toExternalQueue);

	/**
	 * @brief Initialize connections, error aggregators
	 */
	void run();

	/**
	 * @brief Stops connections and joins thread for command handling
	 */
	void destroy();

private:

	/**
	 * @brief Initialize connections with external server
	 */
	void initConnections();

	/**
	 * @brief Start connect sequence with given connection
	 *
	 * @param connection connection which wants to start connect sequence
	 */
	void startExternalConnectSequence(connection::ExternalConnection &connection);

	/**
	 * @brief Handle aggregated status messages from a module handler
	 */
	void handleAggregatedMessages();

	/**
	 * @brief Handle commands messages from from an external server
	 */
	void handleCommands();

	/**
	 * @brief Update command on device
	 *
	 * @param deviceCommand new command which replaces the old one
	 */
	void handleCommand(const InternalProtocol::DeviceCommand &deviceCommand);

	/**
	 * @brief Send aggregated status message to the external server
	 *
	 * @param deviceStatus aggregated status message ready to send
	 * @return reconnect expected if true, reconnect not expected if false
	 */
	bool sendStatus(const structures::InternalClientMessage &deviceStatus);

	bool insideConnectSequence_ { false };

	/**
	 * @brief Map of external connections, key is number from settings
	 * - map is needed because of the possibility of multiple modules connected to one external server
	 */
	std::map<unsigned int, std::reference_wrapper<connection::ExternalConnection>> externalConnectionMap_ {};
	/// List of external connections, each device can have its own connection or multiple devices can share one connection
	std::list<connection::ExternalConnection> externalConnectionsList_ {};
	/// Queue for  messages from module handler to external client to be sent to external server
	std::shared_ptr<structures::AtomicQueue<structures::InternalClientMessage>> toExternalQueue_;
	/// Queue for device commands received by external client to module handler
	std::shared_ptr<structures::AtomicQueue<InternalProtocol::DeviceCommand>> fromExternalQueue_ {};

	std::shared_ptr<structures::AtomicQueue<structures::ReconnectQueueItem>> reconnectQueue_ {};

	std::jthread fromExternalClientThread_ {};

	std::shared_ptr<structures::GlobalContext> context_;

	structures::ModuleLibrary &moduleLibrary_;

	/// Timer for establishing connection with external server
	boost::asio::deadline_timer timer_;
};

}
