#include <testing_utils/CommunicationMock.hpp>


namespace testing_utils {

CommunicationMock::CommunicationMock(const bringauto::structures::ExternalConnectionSettings &settings)
	: ICommunicationChannel(settings) {
}

CommunicationMock::~CommunicationMock() {
	closeConnection();
}

void CommunicationMock::initializeConnection() {
	if (failOnInitConnection_) {
		throw std::runtime_error("Failed to initialize connection");
	}
}

bool CommunicationMock::sendMessage(ExternalProtocol::ExternalClient* message) {
	if (failOnSend_) {
		return false;
	}
	switch (nextMessageType_) {
		case CONNECT_RESPONSE:
			sessionId_ = message->connect().sessionid();
			return true;
		case STATUS_RESPONSE:
			return true;
		case COMMAND:
			return true;
		default:
			return false;
	}
}

std::shared_ptr<ExternalProtocol::ExternalServer> CommunicationMock::receiveMessage() {
	auto ptr = std::make_shared<ExternalProtocol::ExternalServer>();

	switch (nextMessageType_) {
		case CONNECT_RESPONSE:
			if (connectMsgNullptr_) {
				return nullptr;
			}
			if (!connectMsgBadSessionId_) {
				ptr->mutable_connectresponse()->set_sessionid(sessionId_);
			}
			if (connectMsgNoType_) {
				ptr->clear_connectresponse();
			} else if (connectMsgAlreadyLoggedIn_) {
				ptr->mutable_connectresponse()->set_type(ExternalProtocol::ConnectResponse_Type_ALREADY_LOGGED);
			} else {
				ptr->mutable_connectresponse()->set_type(ExternalProtocol::ConnectResponse_Type_OK);
			}
			nextMessageType_ = STATUS_RESPONSE;
			break;

		case STATUS_RESPONSE:
			if (statusMsgNullptr_) {
				return nullptr;
			}
			if (!statusMsgBadSessionId_) {
				ptr->mutable_statusresponse()->set_sessionid(sessionId_);
			}
			if (statusMsgNoType_) {
				ptr->clear_statusresponse();
			} else if (statusMsgNotOk_) {
				ptr->mutable_statusresponse()->set_type(
					ExternalProtocol::StatusResponse_Type_StatusResponse_Type_INT_MAX_SENTINEL_DO_NOT_USE_
				);
			} else {
				ptr->mutable_statusresponse()->set_type(ExternalProtocol::StatusResponse_Type_OK);
				ptr->mutable_statusresponse()->set_messagecounter(0);
			}
			nextMessageType_ = COMMAND;
			break;

		case COMMAND:
			if (commandMsgNullptr_) {
				return nullptr;
			}
			if (!commandMsgBadSessionId_) {
				ptr->mutable_command()->set_sessionid(sessionId_);
			}
			if (commandMsgNoType_) {
				ptr->clear_command();
			} else {
				ptr->mutable_command()->set_messagecounter(0);
				ptr->mutable_command()->mutable_devicecommand()->mutable_device()->set_devicename("device");
				ptr->mutable_command()->mutable_devicecommand()->mutable_device()->set_devicerole("name");
				if (commandMsgModuleNotExists_) {
					ptr->mutable_command()->mutable_devicecommand()->mutable_device()->set_module(
						InternalProtocol::Device_Module_MISSION_MODULE
					);
				} else {
					ptr->mutable_command()->mutable_devicecommand()->mutable_device()->set_module(
						InternalProtocol::Device_Module_EXAMPLE_MODULE
					);
				}
				ptr->mutable_command()->mutable_devicecommand()->mutable_device()->set_devicetype(0);
				ptr->mutable_command()->mutable_devicecommand()->mutable_device()->set_priority(0);
				ptr->mutable_command()->mutable_devicecommand()->set_commanddata("command");
			}
			nextMessageType_ = CONNECT_RESPONSE;
			break;
	}

	return ptr;
}

void CommunicationMock::closeConnection() {
}

std::string CommunicationMock::getSessionId() const {
	return sessionId_;
}

void CommunicationMock::setFailOnInitConnection(bool failOnInitConnection) {
	failOnInitConnection_ = failOnInitConnection;
}

void CommunicationMock::setFailOnSend(bool failOnSend) {
	failOnSend_ = failOnSend;
}

void CommunicationMock::setConnectMsgNullptr(bool connectMsgNullptr) {
	connectMsgNullptr_ = connectMsgNullptr;
}

void CommunicationMock::setConnectMsgNoType(bool connectMsgNoType) {
	connectMsgNoType_ = connectMsgNoType;
}

void CommunicationMock::setConnectMsgBadSessionId(bool connectMsgBadSessionId) {
	connectMsgBadSessionId_ = connectMsgBadSessionId;
}

void CommunicationMock::setConnectMsgAlreadyLoggedIn(bool connectMsgAlreadyLoggedIn) {
	connectMsgAlreadyLoggedIn_ = connectMsgAlreadyLoggedIn;
}

void CommunicationMock::setStatusMsgNullptr(bool statusMsgNullptr) {
	statusMsgNullptr_ = statusMsgNullptr;
}

void CommunicationMock::setStatusMsgNoType(bool statusMsgNoType) {
	statusMsgNoType_ = statusMsgNoType;
}

void CommunicationMock::setStatusMsgNotOk(bool statusMsgNotOk) {
	statusMsgNotOk_ = statusMsgNotOk;
}

void CommunicationMock::setStatusMsgBadSessionId(bool statusMsgBadSessionId) {
	statusMsgBadSessionId_ = statusMsgBadSessionId;
}

void CommunicationMock::setCommandMsgNullptr(bool commandMsgNullptr) {
	commandMsgNullptr_ = commandMsgNullptr;
}

void CommunicationMock::setCommandMsgNoType(bool commandMsgNoType) {
	commandMsgNoType_ = commandMsgNoType;
}

void CommunicationMock::setCommandMsgBadSessionId(bool commandMsgBadSessionId) {
	commandMsgBadSessionId_ = commandMsgBadSessionId;
}

void CommunicationMock::setCommandMsgModuleNotExists(bool commandMsgModuleNotExists) {
	commandMsgModuleNotExists_ = commandMsgModuleNotExists;
}

}
