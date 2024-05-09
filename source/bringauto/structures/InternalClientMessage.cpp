#include <bringauto/structures/InternalClientMessage.hpp>



namespace bringauto::structures {

const InternalProtocol::InternalClient &InternalClientMessage::getMessage() const {
	return message_;
}

bool InternalClientMessage::disconnected() const {
	return disconnect_;
}

const DeviceIdentification &InternalClientMessage::getDeviceId() const {
	return deviceId_;
}

}