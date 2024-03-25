#pragma once

#include <InternalProtocol.pb.h>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ModuleLibrary.hpp>
#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>
#include <bringauto/structures/ModuleHandlerMessage.hpp>

#include <fleet_protocol/common_headers/device_management.h>

#include <memory>



namespace bringauto::modules {

class ModuleHandler {
public:
	ModuleHandler(
			std::shared_ptr <structures::GlobalContext> &context,
			structures::ModuleLibrary &moduleLibrary,
			std::shared_ptr <structures::AtomicQueue<structures::InternalClientMessage>> &fromInternalQueue,
			std::shared_ptr <structures::AtomicQueue<structures::ModuleHandlerMessage>> &toInternalQueue,
			std::shared_ptr <structures::AtomicQueue<structures::InternalClientMessage>> &toExternalQueue)
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
	 * @brief Process all incoming messages from internal server
	 *
	 */
	void handleMessages();

	/**
	 * @brief Check if there are any timeouted messages
	 *
	 */
	void checkTimeoutedMessages();

	/**
	 * @brief Process disconnect device
	 *
	 * @param device device identification
	 */
	void handleDisconnect(device_identification deviceId);

	/**
	 * @brief Send aggregated status to external server
	 *
	 * @param deviceId device identification
	 * @param device protobuf device
	 * @param disconnected true if device is disconnected otherwise false
	 */
	void sendAggregatedStatus(const device_identification &deviceId, const InternalProtocol::Device &device, bool disconnected);

	/**
	 * @brief Process connect message
	 *
	 * @param connect Connect message
	 */
	void handleConnect(const InternalProtocol::DeviceConnect &connect);

	/**
	 * @brief Send connect response message to internal client
	 *
	 * @param device internal client
	 * @param response_type type of the response
	 */
	void sendConnectResponse(const InternalProtocol::Device &device, InternalProtocol::DeviceConnectResponse_ResponseType response_type);

	/**
	 * @brief Process status message
	 *
	 * @param status Status message
	 */
	void handleStatus(const InternalProtocol::DeviceStatus &status);

	std::shared_ptr <structures::GlobalContext> context_;

	structures::ModuleLibrary &moduleLibrary_;
	/// Queue for incoming messages from internal server to be processed
	std::shared_ptr <structures::AtomicQueue<structures::InternalClientMessage>> fromInternalQueue_;
	/// Queue for outgoing messages to internal server to be forwarded to devices
	std::shared_ptr <structures::AtomicQueue<structures::ModuleHandlerMessage>> toInternalQueue_;
	/// Queue for outgoing messages to external server to be forwarded to external server
	std::shared_ptr <structures::AtomicQueue<structures::InternalClientMessage>> toExternalQueue_;
};

}
