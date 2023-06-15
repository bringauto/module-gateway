#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <mqtt/async_client.h>



namespace bringauto::external_client::connection::communication {

class MqttCommunication : ICommunicationChannel {
public:
	explicit MqttCommunication(structures::ExternalConnectionSettings &settings, std::string company,
							   std::string vehicleName);

    void init();

	~MqttCommunication() override;

	int initializeConnection() override;

	int sendMessage() override;

	// TODO getCommand, or just add Commands to CommandQueue from listening thread

	void closeConnection() override;

	static std::string createClientId(const std::string& company, const std::string& vehicleName);

private:

	void connect(std::string topic);

	static std::string createPublishTopic(const std::string& company, const std::string& vehicleName);
	static std::string createSubscribeTopic(const std::string& company, const std::string& vehicleName);

	/**
	 * MQTT client
	 */
	std::unique_ptr<mqtt::async_client> client_ { nullptr };

	std::string publishTopic_ {};

	std::string subscribeTopic_ {};
};

}