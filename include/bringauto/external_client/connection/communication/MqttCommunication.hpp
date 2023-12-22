#pragma once

#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <mqtt/async_client.h>

#include <string>



namespace bringauto::external_client::connection::communication {

class MqttCommunication: public ICommunicationChannel {
public:
	explicit MqttCommunication(const structures::ExternalConnectionSettings &settings): ICommunicationChannel(
			settings) {};

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
	std::string createClientId(const std::string &company, const std::string &vehicleName);

	/**
	 * @brief Create a publish topic from company name and vehicle name
	 *
	 * @param company name of the company
	 * @param vehicleName name of the vehicle
	 * @return std::string
	 */
	std::string createPublishTopic(const std::string &company, const std::string &vehicleName);

	/**
	 * @brief Create a subscribe topic from company name and vehicle name
	 *
	 * @param company name of the company
	 * @param vehicleName name of the vehicle
	 * @return std::string
	 */
	std::string createSubscribeTopic(const std::string &company, const std::string &vehicleName);

	/**
	 * MQTT client
	 */
	std::unique_ptr<mqtt::async_client> client_ { nullptr };

	std::string clientId_ {};

	std::string publishTopic_ {};

	std::string subscribeTopic_ {};

	mqtt::connect_options connopts_ {};

	std::string serverAddress_ {};

	/**
	 * MQTT QOS level. Level 0 has no assurance of delivery and does not buffer messages.
	 */
	constexpr static int8_t qos { 0 };
};

}