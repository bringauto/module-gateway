#include <bringauto/structures/ModuleHandlerMessage.hpp>



namespace bringauto::structures {

const InternalProtocol::InternalServer &ModuleHandlerMessage::getMessage() const {
	return message_;
}

bool ModuleHandlerMessage::disconnected() const {
	return disconnect_;
}

const device_identification &ModuleHandlerMessage::getDeviceId() const {
	return deviceId_;
}

};