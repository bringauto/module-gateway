#pragma once

#include <memory>
#include <unordered_map>

#include <InternalProtocol.pb.h>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <device_management.h>



namespace bringauto::modules {

class ModuleHandler {

public:
	ModuleHandler(
			std::shared_ptr <structures::GlobalContext> &context,
			std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> &fromInternalQueue,
			std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalServer>> &toInternalQueue,
			std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> &toExternalQueue)
			: context_ { context }, fromInternalQueue_ { fromInternalQueue }, toInternalQueue_ { toInternalQueue },
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
	 * @brief Initialize all modules defined in config file
	 *
	 */
	void init_modules();

	/**
	 * @brief Initialize single module
	 *
	 * @param path path to the module
	 */
	void init_module(const std::string &path);

	/**
	 * @brief Process all incoming messages
	 *
	 */
	void handle_messages();

	/**
	 * @brief Process connect message
	 *
	 * @param connect Connect message
	 */
	void handle_connect(const InternalProtocol::DeviceConnect &connect);

	/**
	 * @brief Process status message
	 *
	 * @param status Status message
	 */
	void handle_status(const InternalProtocol::DeviceStatus &status);

	/**
	 * @brief Creates device_identification from protobuf device
	 *
	 * @param device internal protocol device
	 * @return ::device_identification
	 */
	::device_identification mapToDeviceId(const InternalProtocol::Device &device);

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> fromInternalQueue_;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalServer>> toInternalQueue_;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> toExternalQueue_;

	std::shared_ptr <structures::GlobalContext> context_;

	// std::unordered_map<unsigned int, StatusAggregator> modules_ {};
};

}