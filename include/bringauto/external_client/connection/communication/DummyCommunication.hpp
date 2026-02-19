#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>



namespace bringauto::external_client::connection::communication {

/**
 * @brief Dummy communication channel for unit-test and send-only scenarios.
 * Does not establish any real connection, just simulates it.
 *
 * Constraints:
 * - receiveMessage() always returns nullptr.
 * - Because ExternalConnection's connect sequence relies on receiving a valid
 *   server response, DUMMY can never complete a connect sequence. Any gateway
 *   instance configured with protocol-type "DUMMY" will loop reconnecting
 *   indefinitely. Use only in unit tests or when the connect sequence is
 *   bypassed by the test harness.
 * - initializeConnection() and closeConnection() only toggle an internal flag
 *   used to gate sendMessage() log output.
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
