#include <bringauto/external_client/connection/messages/SentMessagesHandler.hpp>
#include <bringauto/logging/Logger.hpp>

#include <fleet_protocol/common_headers/general_error_codes.h>
#include <google/protobuf/util/message_differencer.h>



namespace bringauto::external_client::connection::messages {


SentMessagesHandler::SentMessagesHandler(const std::shared_ptr<structures::GlobalContext> &context,
										 const std::function<void()> &endConnectionFunc): context_ { context },
																						 endConnectionFunc_ {
																								 endConnectionFunc } {}

void SentMessagesHandler::addNotAckedStatus(const ExternalProtocol::Status &status) {
	std::scoped_lock lock {ackMutex_};
	notAckedStatuses_.emplace_back(
			std::make_shared<NotAckedStatus>(status, context_->ioContext, responseHandled_, responseHandledMutex_));
	notAckedStatuses_.back()->startTimer(endConnectionFunc_);
}

int SentMessagesHandler::acknowledgeStatus(const ExternalProtocol::StatusResponse &statusResponse) {
	std::scoped_lock lock {ackMutex_};
	auto responseCounter = getStatusResponseCounter(statusResponse);
	for(auto i = 0; i < notAckedStatuses_.size(); ++i) {
		if(getStatusCounter(notAckedStatuses_[i]->getStatus()) == responseCounter) {
			notAckedStatuses_[i]->cancelTimer();
			notAckedStatuses_.erase(notAckedStatuses_.begin() + i);
			if(not isAnyDeviceConnected() && allStatusesAcked()) {
				return NOT_OK; //maybe change to other and not NOT_OK
			}
			return OK;
		}
	}
	return NOT_OK;
}

const std::vector<std::shared_ptr<NotAckedStatus>> &SentMessagesHandler::getNotAckedStatuses() const {
	return notAckedStatuses_;
}

bool SentMessagesHandler::allStatusesAcked() const {
	return notAckedStatuses_.empty();
}

void SentMessagesHandler::clearAll() {
	clearAllTimers();
	notAckedStatuses_.clear();
}

void SentMessagesHandler::addDeviceAsConnected(const structures::DeviceIdentification &device) {
	connectedDevices_.push_back(device);
}

void SentMessagesHandler::deleteConnectedDevice(const structures::DeviceIdentification &device) {
	auto it = std::find(connectedDevices_.begin(), connectedDevices_.end(), device);
	if(it != connectedDevices_.end()) {
		connectedDevices_.erase(it);
	} else {
		logging::Logger::logError("Trying to delete not connected device id: {}", device.convertToString());
	}
}

bool SentMessagesHandler::isDeviceConnected(const structures::DeviceIdentification &device) {
	return std::any_of(connectedDevices_.begin(), connectedDevices_.end(), [&](const auto &connectedDevice) {
		return device == connectedDevice;
	});
}

bool SentMessagesHandler::isAnyDeviceConnected() const {
	return not connectedDevices_.empty();
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
