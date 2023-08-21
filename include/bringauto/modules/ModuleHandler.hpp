#pragma once

#include <InternalProtocol.pb.h>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ModuleLibrary.hpp>
#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>
#include <device_management.h>

#include <memory>



namespace bringauto::modules {

class ModuleHandler {

public:
	ModuleHandler(
			std::shared_ptr <structures::GlobalContext> &context,
			structures::ModuleLibrary &moduleLibrary,
			std::shared_ptr <structures::AtomicQueue<structures::InternalClientMessage>> &fromInternalQueue,
			std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalServer>> &toInternalQueue,
			std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> &toExternalQueue)
			: context_ { context }, moduleLibrary_ { moduleLibrary }, fromInternalQueue_ { fromInternalQueue },
			  toInternalQueue_ { toInternalQueue },
			  toExternalQueue_ { toExternalQueue } {}

	/**
	 * @brief Start Module handler
	 *
	 * Initializes all modules and handles incoming messages from Internal Server through queues
	 *
	 */
	void run();

	/**
	 * @brief Stop Module handler and clean all initialized modules
	 *
	 */
	void destroy();

private:

	/**
	 * @brief Process all incoming messages
	 *
	 */
	void handle_messages();

    /**
     * @brief Process disconnect device
     *
     * @param device device identification
     */
    void handleDisconnect(device_identification device);

	/**
	 * @brief Process connect message
	 *
	 * @param connect Connect message
	 */
	void handleConnect(const InternalProtocol::DeviceConnect &connect);

	/**
	 * @brief Process status message
	 *
	 * @param status Status message
	 */
	void handleStatus(const InternalProtocol::DeviceStatus &status);

	std::shared_ptr <structures::GlobalContext> context_;

	structures::ModuleLibrary &moduleLibrary_;

	std::shared_ptr <structures::AtomicQueue<structures::InternalClientMessage>> fromInternalQueue_;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalServer>> toInternalQueue_;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> toExternalQueue_;
};

}