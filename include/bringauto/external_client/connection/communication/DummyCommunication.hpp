#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>



namespace bringauto::external_client::connection::communication {

/**
 * @brief Dummy communication channel for testing purposes.
 * Does not establish any real connection, just simulates it.
 * receiveMessage always returns nullptr.
 * Initialization and closing connection only changes the debug logs of sendMessage.
 */
class DummyCommunication: public ICommunicationChannel {
public:
	explicit DummyCommunication(const structures::ExternalConnectionSettings &settings);

	~DummyCommunication() override;

	void initializeConnection() override;

	bool sendMessage(ExternalProtocol::ExternalClient *message) override;

	std::shared_ptr<ExternalProtocol::ExternalServer> receiveMessage() override;

	void closeConnection() override;

private:
	/// Flag to indicate if the fake connection is established
	bool isConnected_ { false };
};

}
