#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>



namespace bringauto::external_client::connection::communication {

class DummyCommunication: public ICommunicationChannel {
public:
	explicit DummyCommunication(const structures::ExternalConnectionSettings &settings);

	~DummyCommunication() override;

	void initializeConnection() override;

	bool sendMessage(ExternalProtocol::ExternalClient *message) override;

	std::shared_ptr<ExternalProtocol::ExternalServer> receiveMessage() override;

	void closeConnection() override;

protected:
	/// Flag to indicate if the fake connection is established
	bool isConnected_ { false };
};

}
