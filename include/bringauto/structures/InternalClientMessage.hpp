#pragma once

#include <device_management.h>

#include <InternalProtocol.pb.h>



namespace bringauto::structures {

class InternalClientMessage {

public:
	explicit InternalClientMessage(const device_identification &deviceId): disconnect_ { true }, deviceId_ { deviceId } {};

	explicit InternalClientMessage(const InternalProtocol::InternalClient &message): message_ { message } {}

	const InternalProtocol::InternalClient &getMessage() const;

	bool disconnected() const;

	const device_identification &getDeviceId() const;

private:

	InternalProtocol::InternalClient message_;

	bool disconnect_ { false };

	device_identification deviceId_;
};

}
