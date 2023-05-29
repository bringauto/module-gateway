#include <testing_utils/ModuleHandlerForTesting.hpp>



namespace testing_utils {
namespace common_utils = bringauto::common_utils;

void ModuleHandlerForTesting::start() {
	size_t messageCounter { 0 };
	while(!context->ioContext.stopped()) {
		if(!fromInternalQueue_->waitForValueWithTimeout(queue_timeout_length)) {
			auto message = fromInternalQueue_->front();
			fromInternalQueue_->pop();
			if(message.has_deviceconnect()) {
				auto res = common_utils::ProtocolUtils::CreateServerMessage(message.deviceconnect().device(),
																				 InternalProtocol::DeviceConnectResponse_ResponseType_OK);
				toInternalQueue_->pushAndNotify(res);
			}
			if(message.has_devicestatus()) {
				auto com = common_utils::ProtocolUtils::CreateServerMessage(message.devicestatus().device(),
																				 message.devicestatus().statusdata());
				toInternalQueue_->pushAndNotify(com);
			}
			++messageCounter;
		}
	}
	ASSERT_EQ(messageCounter, expectedMessageNumber);
}

void ModuleHandlerForTesting::startWithTimeout(bool onConnect, size_t timeoutNumber) {
	size_t messageCounter { 0 };
	while(!context->ioContext.stopped()) {
		if(!fromInternalQueue_->waitForValueWithTimeout(queue_timeout_length)) {
			auto message = fromInternalQueue_->front();
			fromInternalQueue_->pop();
			if(message.has_deviceconnect()) {
				if(onConnect && timeoutNumber > 0) {
					--timeoutNumber;
					continue;
				}
				auto res = common_utils::ProtocolUtils::CreateServerMessage(message.deviceconnect().device(),
																				 InternalProtocol::DeviceConnectResponse_ResponseType_OK);
				toInternalQueue_->pushAndNotify(res);
			}
			if(message.has_devicestatus()) {
				if(!onConnect && timeoutNumber > 0) {
					--timeoutNumber;
					continue;
				}
				auto com = common_utils::ProtocolUtils::CreateServerMessage(message.devicestatus().device(),
																				 message.devicestatus().statusdata());
				toInternalQueue_->pushAndNotify(com);
			}
			++messageCounter;
		}
	}
	ASSERT_EQ(messageCounter, expectedMessageNumber);
}

}