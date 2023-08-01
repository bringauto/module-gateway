#include <bringauto/external_client/connection/messages/SentMessagesHandler.hpp>

#include <google/protobuf/util/message_differencer.h>
#include <bringauto/logging/Logger.hpp>
#include <utility>



namespace bringauto::external_client::connection::messages {


SentMessagesHandler::SentMessagesHandler(const std::shared_ptr <structures::GlobalContext> &context,
										 const std::function<void()> endConnectionFunc): context_ { context },
																						 endConnectionFunc_ {
																								 endConnectionFunc } {}

void SentMessagesHandler::addNotAckedStatus(const ExternalProtocol::Status &status) {
	notAckedStatuses_.emplace_back(
			std::make_shared<NotAckedStatus>(status, context_->ioContext, responseHandled_, responseHandledMutex_));
	notAckedStatuses_.back()->startTimer(endConnectionFunc_);
}

int SentMessagesHandler::acknowledgeStatus(const ExternalProtocol::StatusResponse &statusResponse) {
	auto responseCounter = getStatusResponseCounter(statusResponse);
	for(auto i = 0; i < notAckedStatuses_.size(); ++i) {
		if(getStatusCounter(notAckedStatuses_[i]->getStatus()) == responseCounter) {
			notAckedStatuses_[i]->cancelTimer();
			notAckedStatuses_.erase(notAckedStatuses_.begin() + i);
			return OK;
		}
	}
	return NOT_OK;
}

void SentMessagesHandler::clearAll() {
	clearAllTimers();
	notAckedStatuses_.clear();
}

void SentMessagesHandler::addDeviceAsConnected(const InternalProtocol::Device &device) {
	connectedDevices_.push_back(device);
}

void SentMessagesHandler::deleteConnectedDevice(const InternalProtocol::Device &device) {
	for(auto i = 0; i < connectedDevices_.size(); ++i) {
		if(google::protobuf::util::MessageDifferencer::Equals(device, connectedDevices_[i])) {
			connectedDevices_.erase(connectedDevices_.begin() + i);
			return;
		}
	}
	logging::Logger::logError("Trying to delete not connected device. role: {}, name: {}", device.devicerole(),
							  device.devicename());
}

bool SentMessagesHandler::isDeviceConnected(InternalProtocol::Device device) {
	return std::any_of(connectedDevices_.begin(), connectedDevices_.end(), [&](const auto &connectedDevice) {
		return google::protobuf::util::MessageDifferencer::Equals(device, connectedDevice);
	});
}

void SentMessagesHandler::clearAllTimers() {
	for(auto &notAckedStatus: notAckedStatuses_) {
		notAckedStatus->cancelTimer();
	}
	responseHandled_ = false;
}

u_int32_t SentMessagesHandler::getStatusCounter(const ExternalProtocol::Status &status) {
	return status.messagecounter();
}

u_int32_t SentMessagesHandler::getStatusResponseCounter(const ExternalProtocol::StatusResponse &statusResponse) {
	return statusResponse.messagecounter();
}

}