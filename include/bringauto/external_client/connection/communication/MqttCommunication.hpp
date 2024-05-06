#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <mqtt/async_client.h>

#include <string>



namespace bringauto::external_client::connection::communication {

class MqttCommunication: public ICommunicationChannel {
public:
	explicit MqttCommunication(const structures::ExternalConnectionSettings &settings);

	~MqttCommunication() override;

	void setProperties(const std::string &company, const std::string &vehicleName) override;

	void initializeConnection() override;

	bool sendMessage(ExternalProtocol::ExternalClient *message) override;

	std::shared_ptr<ExternalProtocol::ExternalServer> receiveMessage() override;

	void closeConnection() override;

private:
	void connect();

	/**
	 * @brief Create a client id from company name and vehicle name
	 *
	 * @param company name of the company
	 * @param vehicleName name of the vehicle
	 * @return std::string
	 */
	static std::string createClientId(const std::string &company, const std::string &vehicleName);

	/**
	 * @brief Create a publish topic from company name and vehicle name
	 *
	 * @param company name of the company
	 * @param vehicleName name of the vehicle
	 * @return std::string
	 */
	static std::string createPublishTopic(const std::string &company, const std::string &vehicleName);

	/**
	 * @brief Create a subscribe topic from company name and vehicle name
	 *
	 * @param company name of the company
	 * @param vehicleName name of the vehicle
	 * @return std::string
	 */
	static std::string createSubscribeTopic(const std::string &company, const std::string &vehicleName);

	/// MQTT client handling the connection
	std::unique_ptr<mqtt::async_client> client_ { nullptr };
	/// Unique ID of the client, changes with every connection
	std::string clientId_ {};
	/// Topic to publish messages to, sender is external client, receiver is external server
	std::string publishTopic_ {};
	/// Topic to subscribe to, sender is external server, receiver is external client
	std::string subscribeTopic_ {};
	/// MQTT library connection options
	mqtt::connect_options connopts_ {};
	/// Address of the MQTT server
	std::string serverAddress_ {};
	/// MQTT QOS level. Level 1 assures that message is delivered at least once
	constexpr static int8_t qos { 1 };
};

}
