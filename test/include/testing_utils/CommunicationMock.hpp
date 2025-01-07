#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <string>


namespace testing_utils {

class CommunicationMock: public bringauto::external_client::connection::communication::ICommunicationChannel {
public:
	explicit CommunicationMock(const bringauto::structures::ExternalConnectionSettings &settings);

	~CommunicationMock() override;

	void setProperties(const std::string &company, const std::string &vehicleName) override;

	void initializeConnection() override;

	bool sendMessage(ExternalProtocol::ExternalClient *message) override;

	std::shared_ptr<ExternalProtocol::ExternalServer> receiveMessage() override;

	void closeConnection() override;

	std::string getSessionId() const;

	void setFailOnInitConnection(bool failOnInitConnection);
	void setFailOnSend(bool failOnSend);

	void setConnectMsgNullptr(bool connectMsgNullptr);
	void setConnectMsgNoType(bool connectMsgNoType);
	void setConnectMsgBadSessionId(bool connectMsgBadSessionId);
	void setConnectMsgAlreadyLoggedIn(bool connectMsgAlreadyLoggedIn);

	void setStatusMsgNullptr(bool statusMsgNullptr);
	void setStatusMsgNoType(bool statusMsgNoType);
	void setStatusMsgNotOk(bool statusMsgNotOk);
	void setStatusMsgBadSessionId(bool statusMsgBadSessionId);

	void setCommandMsgNullptr(bool commandMsgNullptr);
	void setCommandMsgNoType(bool commandMsgNoType);
	void setCommandMsgBadSessionId(bool commandMsgBadSessionId);
	void setCommandMsgModuleNotExists(bool commandMsgModuleNotExists);

private:
	enum NextMessageType {
		CONNECT_RESPONSE,
		STATUS_RESPONSE,
		COMMAND
	} nextMessageType_ { CONNECT_RESPONSE };

	std::string sessionId_ {};
	bool failOnInitConnection_ { false };
	bool failOnSend_ { false };

	bool connectMsgNullptr_ { false };
	bool connectMsgNoType_ { false };
	bool connectMsgBadSessionId_ { false };
	bool connectMsgAlreadyLoggedIn_ { false };

	bool statusMsgNullptr_ { false };
	bool statusMsgNoType_ { false };
	bool statusMsgNotOk_ { false };
	bool statusMsgBadSessionId_ { false };

	bool commandMsgNullptr_ { false };
	bool commandMsgNoType_ { false };
	bool commandMsgBadSessionId_ { false };
	bool commandMsgModuleNotExists_ { false };
};

}
