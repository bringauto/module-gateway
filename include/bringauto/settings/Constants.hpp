#pragma once

#include <string_view>
#include <chrono>



namespace bringauto::settings {

/**
 * @brief timeout that is defined in fleet protocol,
 * active connection between Internal Client and internal Server is disconnected and removed after this timeout
 */
constexpr std::chrono::milliseconds fleet_protocol_timeout_length { 250 };

/**
 * @brief timeout that is defined in fleet protocol,
 * reconnect time between External client and External server after disconnect
 */
constexpr int reconnect_delay { 10 };

/**
 * @brief timeout that defines force aggregation on device
 */
constexpr std::chrono::seconds status_aggregation_timeout { 15 };

/**
 * @brief max amount of status aggregation timeouts before device is considered disconnected
 */
constexpr int status_aggregation_timeout_max_count { 2 };

/**
 * @brief timeout that defines how much time can be status without status response
 * if timeout expired, it evokes connection disconnect
 */
constexpr int status_response_timeout { 30 };

/**
 * @brief time between checks of atomic queue used for one-way communication from Module Handler to Internal Server
 */
constexpr std::chrono::seconds queue_timeout_length { 3 };

/**
 * @brief timeout to wait on receive message for external client transport layer
 */
constexpr std::chrono::seconds receive_message_timeout { 5 };

/**
 * @brief timeout for immediate disconnect
 */
constexpr std::chrono::seconds immediate_disconnect_timeout { 10 };

/**
 * &brief Fleet Protocol defines messages as always starting with 4 bytes header
 */
constexpr uint8_t header { 4 };

/**
 * @brief maximal amount of bytes received, that can be processed in one cycle
 */
constexpr size_t buffer_length { 1024 };

/**
 * @brief Constants for Mqtt communication
*/
struct MqttConstants {
	/**
	 * @brief keep alive interval in seconds;
	 *        value reasoning: keepalive is half of the default timeout in Fleet protocol
	*/
	static constexpr uint8_t keepalive { status_response_timeout/2 };

	/**
	 * @brief automatic reconnection of mqtt client option
	*/
	static constexpr bool automatic_reconnect { true };

	/**
	 * @brief max time that the mqtt client will wait for a connection before failing;
	 *        value reasoning: TCP timeout for retransmission when TCP packet is dropped is 200ms, this value is multiple of three of this value
	*/
	static constexpr size_t connect_timeout { 600 };

	/**
	 * @brief max messages that can be in the process of transmission simultaneously;
	 *        value reasoning: module gateway can handle cca 20 devices
	*/
	static constexpr size_t max_inflight { 20 };
};

/**
 * @brief Constant string views
 */
class Constants {
public:
	inline static constexpr std::string_view CONFIG_PATH { "config-path" };
	inline static constexpr std::string_view VERBOSE { "verbose" };
	inline static constexpr std::string_view LOG_PATH { "log-path" };
	inline static constexpr std::string_view HELP { "help" };
	inline static constexpr std::string_view PORT { "port" };

	inline static constexpr std::string_view MODULE_PATHS { "module-paths" };

	inline static constexpr std::string_view GENERAL_SETTINGS { "general-settings" };
	inline static constexpr std::string_view INTERNAL_SERVER_SETTINGS { "internal-server-settings" };

	inline static constexpr std::string_view EXTERNAL_CONNECTION { "external-connection" };
	inline static constexpr std::string_view VEHICLE_NAME { "vehicle-name" };
	inline static constexpr std::string_view COMPANY { "company" };
	inline static constexpr std::string_view EXTERNAL_ENDPOINTS { "endpoints" };
	inline static constexpr std::string_view SERVER_IP { "server-ip" };
	inline static constexpr std::string_view PROTOCOL_TYPE { "protocol-type" };

	inline static constexpr std::string_view MQTT { "MQTT" };
	inline static constexpr std::string_view MQTT_SETTINGS { "mqtt-settings" };
	inline static constexpr std::string_view SSL { "ssl" };
	inline static constexpr std::string_view CA_FILE { "ca-file" };
	inline static constexpr std::string_view CLIENT_CERT { "client-cert" };
	inline static constexpr std::string_view CLIENT_KEY { "client-key" };

	inline static constexpr std::string_view MODULES { "modules" };
};

}