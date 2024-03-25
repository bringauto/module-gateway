#include <testing_utils/ModuleHandlerForTesting.hpp>
#include <testing_utils/ProtobufUtils.hpp>


namespace testing_utils {

void ModuleHandlerForTesting::start() {
	size_t messageCounter { 0 };
	while(!context->ioContext.stopped()) {
		if(!fromInternalQueue_->waitForValueWithTimeout(queue_timeout_length)) {
			if(fromInternalQueue_->front().disconnected()){
				fromInternalQueue_->pop();
				continue;
			}
			auto message = fromInternalQueue_->front().getMessage();
			fromInternalQueue_->pop();
			if(message.has_deviceconnect()) {
				auto res = ProtobufUtils::CreateServerMessage(
					message.deviceconnect().device(),
					InternalProtocol::DeviceConnectResponse_ResponseType_OK
				);
				auto resModuleHandlerMessage = bringauto::structures::ModuleHandlerMessage(false, res);
				toInternalQueue_->pushAndNotify(resModuleHandlerMessage);
			}
			if(message.has_devicestatus()) {
				auto com = ProtobufUtils::CreateServerMessage(
					message.devicestatus().device(),
					message.devicestatus().statusdata()
				);
				auto comModuleHandlerMessage = bringauto::structures::ModuleHandlerMessage(false, com);
				toInternalQueue_->pushAndNotify(comModuleHandlerMessage);
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
			if(fromInternalQueue_->front().disconnected()){
				fromInternalQueue_->pop();
				continue;
			}
			auto message = fromInternalQueue_->front().getMessage();
			fromInternalQueue_->pop();
			if(message.has_deviceconnect()) {
				if(onConnect && timeoutNumber > 0) {
					--timeoutNumber;
					continue;
				}
				auto res = ProtobufUtils::CreateServerMessage(
					message.deviceconnect().device(),
					InternalProtocol::DeviceConnectResponse_ResponseType_OK
				);
				auto resModuleHandlerMessage = bringauto::structures::ModuleHandlerMessage(false, res);
				toInternalQueue_->pushAndNotify(resModuleHandlerMessage);
			}
			if(message.has_devicestatus()) {
				if(!onConnect && timeoutNumber > 0) {
					--timeoutNumber;
					continue;
				}
				auto com = ProtobufUtils::CreateServerMessage(
					message.devicestatus().device(),
					message.devicestatus().statusdata()
				);
				auto comModuleHandlerMessage = bringauto::structures::ModuleHandlerMessage(false, com);
				toInternalQueue_->pushAndNotify(comModuleHandlerMessage);
			}
			++messageCounter;
		}
	}
	ASSERT_EQ(messageCounter, expectedMessageNumber);
}

}