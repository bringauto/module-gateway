#pragma once

#include <condition_variable>
#include <bringauto/external_client/connection/communication/ICommunicationChannel.hpp>

#include <msquic.h>
#include <queue>
#include <thread>

namespace bringauto::external_client::connection::communication {

class QuicCommunication: public ICommunicationChannel {
public:
	explicit QuicCommunication(const structures::ExternalConnectionSettings &settings, const std::string &company,
		const std::string &vehicleName);

	~QuicCommunication() override;

	void initializeConnection() override;

	bool sendMessage(ExternalProtocol::ExternalClient *message) override;

	std::shared_ptr<ExternalProtocol::ExternalServer> receiveMessage() override;

	void closeConnection() override;

private:
	enum class ConnectionState : uint8_t { DISCONNECTED, CONNECTING, CONNECTED, CLOSING };

	const QUIC_API_TABLE* quic_ { nullptr };
	HQUIC registration_ { nullptr };
	HQUIC config_ { nullptr };
	HQUIC connection_ { nullptr };
	std::string alpn_;
	QUIC_BUFFER alpnBuffer_ {};

	std::string certFile_;
    std::string keyFile_;
	std::string caFile_;

	std::atomic<ConnectionState> connectionState_ { ConnectionState::DISCONNECTED };

	// inbound (server → client)
	std::queue<std::shared_ptr<ExternalProtocol::ExternalServer>> inboundQueue_;
	std::mutex inboundMutex_;
	std::condition_variable inboundCv_;

	// outbound (client → server)
	std::queue<std::shared_ptr<ExternalProtocol::ExternalClient>> outboundQueue_;
	std::mutex outboundMutex_;
	std::condition_variable outboundCv_;

	std::jthread senderThread_;

	/// ---------- Connection ----------
	void loadMsQuic();
	void initRegistration(const char *appName);
	void initConfiguration();
	void configurationOpen(const QUIC_SETTINGS *settings);
	void configurationLoadCredential(const QUIC_CREDENTIAL_CONFIG *credential) const;
	void stop();

	void onMessageDecoded(std::shared_ptr<ExternalProtocol::ExternalServer> msg);

	bool sendViaQuicStream(const std::shared_ptr<ExternalProtocol::ExternalClient> &message);

	/// ---------- Closing client ----------
	void closeMsQuic();
	void closeConfiguration();
	void closeRegistration();

	/// ---------- Callbacks ----------
	static QUIC_STATUS QUIC_API connectionCallback(HQUIC connection, void *context, QUIC_CONNECTION_EVENT *event);
	static unsigned int streamCallback(HQUIC stream, void *context, QUIC_STREAM_EVENT *event);

	void senderLoop();

	/// void connect();

	// /**
	//  * @brief Create a client id from company name and vehicle name
	//  *
	//  * @param company name of the company
	//  * @param vehicleName name of the vehicle
	//  * @return std::string
	//  */
	// static std::string createClientId(const std::string &company, const std::string &vehicleName);

	// /**
	//  * @brief Create a publish topic from company name and vehicle name
	//  *
	//  * @param company name of the company
	//  * @param vehicleName name of the vehicle
	//  * @return std::string
	//  */
	// static std::string createPublishTopic(const std::string &company, const std::string &vehicleName);

	// /**
	//  * @brief Create a subscribe topic from company name and vehicle name
	//  *
	//  * @param company name of the company
	//  * @param vehicleName name of the vehicle
	//  * @return std::string
	//  */
	// static std::string createSubscribeTopic(const std::string &company, const std::string &vehicleName);

	// /// MQTT client handling the connection
	// std::unique_ptr<mqtt::async_client> client_ { nullptr };
	// /// Unique ID of the client, changes with every connection
	// std::string clientId_ {};
	// /// Topic to publish messages to, sender is external client, receiver is external server
	// std::string publishTopic_ {};
	// /// Topic to subscribe to, sender is external server, receiver is external client
	// std::string subscribeTopic_ {};
	// /// MQTT library connection options
	// mqtt::connect_options connopts_ {};
	// /// Address of the MQTT server
	// std::string serverAddress_ {};
	// /// MQTT QOS level. Level 1 assures that message is delivered at least once
	// constexpr static int8_t qos { 1 };
	// /// Mutex to prevent deadlocks when receiving messages
	// std::mutex receiveMessageMutex_ {};

	static const char* toString(ConnectionState state) {
		switch (state) {
			case ConnectionState::DISCONNECTED: return "Disconnected";
			case ConnectionState::CONNECTING:   return "Connecting";
			case ConnectionState::CONNECTED:    return "Connected";
			case ConnectionState::CLOSING:      return "Closing";
			default:                            return "Unknown";
		}
	}
};

}
