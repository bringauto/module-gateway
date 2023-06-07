#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <mqtt/async_client.h>



namespace bringauto::external_client::connection::communication {

class MqttCommunication : ICommunicationChannel {
public:
	explicit MqttCommunication(structures::ExternalConnectionSettings settings);

	~MqttCommunication() override;

	int initializeConnection() override;

	int sendMessage() override;

	// TODO getCommand, or just add Commands to CommandQueue from listening thread

	void closeConnection() override;

private:

	void connect();

	/**
	 * MQTT client
	 */
	std::unique_ptr<mqtt::async_client> client_ { nullptr };
};

}