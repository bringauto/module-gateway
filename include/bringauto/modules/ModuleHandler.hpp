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
			std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalServer>> &toInternalQueue)
			: context_ { context }, fromInternalQueue_ { fromInternalQueue }, toInternalQueue_ { toInternalQueue } {}

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
	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> fromInternalQueue_;

	std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalServer>> toInternalQueue_;

	std::shared_ptr <structures::GlobalContext> context_;

	std::unordered_map<unsigned int, StatusAggregator> modules_ {};

	void init_modules();

	void init_module(const std::string &path);

	void handle_messages();

	void handle_connect(const InternalProtocol::DeviceConnect &connect);

	void handle_status(const InternalProtocol::DeviceStatus &status);

	::device_identification mapToDeviceId(const InternalProtocol::Device &device);
};

}