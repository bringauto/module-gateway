#pragma once

#include <InternalProtocol.pb.h>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>
#include <bringauto/structures/ModuleHandlerMessage.hpp>

#include <gtest/gtest.h>
#include <memory>



namespace testing_utils {
const std::chrono::seconds queue_timeout_length { 3 };

class ModuleHandlerForTesting {
public:
	ModuleHandlerForTesting(
		const std::shared_ptr<bringauto::structures::GlobalContext> &context_,
		const std::shared_ptr<bringauto::structures::AtomicQueue<bringauto::structures::InternalClientMessage>> &fromInternalQueue,
		const std::shared_ptr<bringauto::structures::AtomicQueue<bringauto::structures::ModuleHandlerMessage>> &toInternalQueue,
		size_t num)
		: context(context_), fromInternalQueue_ { fromInternalQueue }, toInternalQueue_ { toInternalQueue },
		expectedMessageNumber(num) {}

	void start();

	void startWithTimeout(bool onConnect, size_t timeoutNumber);

private:
    std::shared_ptr<bringauto::structures::GlobalContext> context;
    std::shared_ptr<bringauto::structures::AtomicQueue<bringauto::structures::InternalClientMessage>> fromInternalQueue_;
    std::shared_ptr<bringauto::structures::AtomicQueue<bringauto::structures::ModuleHandlerMessage>> toInternalQueue_;
    size_t expectedMessageNumber;
};

}