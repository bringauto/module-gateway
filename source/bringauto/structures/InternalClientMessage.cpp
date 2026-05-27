#include <bringauto/structures/InternalClientMessage.hpp>



namespace bringauto::structures {

const InternalProtocol::InternalClient &InternalClientMessage::getMessage() const {
	return message_;
}

bool InternalClientMessage::disconnected() const {
	return disconnect_;
}

bool InternalClientMessage::isCommandForward() const noexcept {
	return commandForward_;
}

const DeviceIdentification &InternalClientMessage::getDeviceId() const {
	return deviceId_;
}

InternalClientMessage InternalClientMessage::makeCommandForward(const DeviceIdentification &deviceId) {
	return InternalClientMessage { deviceId, true };
}

}
