#pragma once

#include <bringauto/modules/Buffer.hpp>

#include <bringauto/async_function_execution/AsyncFunctionExecutor.hpp>
#include <bringauto/fleet_protocol/cxx/BufferAsString.hpp>
#include <bringauto/fleet_protocol/cxx/StringAsBuffer.hpp>

#include <string_view>
#include <chrono>



namespace bringauto::settings {

/**
 * @brief timeout that is defined in Fleet Protocol,
 * active connection between Internal Client and Internal Server is disconnected and removed after this timeout
 */
constexpr std::chrono::milliseconds fleet_protocol_timeout_length { 250 };

/**
 * @brief timeout that is defined in Fleet Protocol,
 * reconnect time between External client and External server after disconnect
 */
constexpr int reconnect_delay { 10 };

/**
 * @brief timeout that defines force aggregation on Device
 */
constexpr std::chrono::seconds status_aggregation_timeout { 15 };

/**
 * @brief max amount of status aggregation timeouts before Device is considered disconnected
 */
constexpr int status_aggregation_timeout_max_count { 2 };

/**
 * @brief timeout that defines how much time can be status without status response
 * if timeout expired, it evokes connection disconnect
 */
constexpr std::chrono::seconds status_response_timeout { 30 };

/**
 * @brief time between checks of atomic queue used for one-way communication from Module Handler to Internal Server
 */
constexpr std::chrono::seconds queue_timeout_length { 3 };

/**
 * @brief timeout to wait on receive message for External Client transport layer
 */
constexpr std::chrono::seconds receive_message_timeout { 5 };

/**
 * @brief timeout for immediate disconnect
 */
constexpr std::chrono::seconds immediate_disconnect_timeout { 10 };

/**
 * @brief Fleet Protocol defines messages as always starting with 4 bytes header
 */
constexpr uint8_t header { 4 };

/**
 * @brief maximum amount of bytes received from Device which can be processed in one cycle
 */
constexpr size_t buffer_length { 1024 };

/**
 * @brief maximal amount of external commands that can be stored in queue
 *        value reasoning: some commands need to be buffered in a queue,
 *        but choosing a high value could introduce a long delay until a device receives a new Command
 *
 *        Commands from External Server are received asynchronously (from Device sent-response chain)
 *        and stored in a queue.
 *
 *        Maximum number of commands in queue reflects maximum number of statuses needs to be sent from device
 *        to Module Gateway to pick up all queued commands and therefore defines maximum response time of Device
 *        on the queued Commands.
 *        NOTE: Device has defined no maximum period for sending Status to Module Gateway - The response time is
 *        completely driven by Device.
 *
 *        Each status send from Device pop command and send it back to Device as a response.
 *        If no commands are left in queue the last command picked up from queue is sent back to Device as a response.
 *
 *        If more than max_external_commands are in queue, the oldest command is removed from queue.
 */
constexpr unsigned int max_external_commands { 3 };

/**
 * @brief how many messages can be in the message queue sent to External Client before it is considered unresponsive
 */
constexpr unsigned int max_external_queue_size { 500 }; 

/**
 * @brief base stream id for Aeron communication from Module Gateway to module binary
 */
constexpr unsigned int aeron_to_module_stream_id_base { 10000 };

/**
 * @brief base stream id for Aeron communication from module binary to Module Gateway
 */
constexpr unsigned int aeron_to_gateway_stream_id_base { 20000 };

/**
 * @brief Constants for Mqtt communication
*/
struct MqttConstants {
	/**
	 * @brief keep alive interval in seconds;
	 *        value reasoning: keepalive is half of the default timeout in Fleet protocol
	 *        The value is chosen based on empiric measurement.
	*/
	static constexpr std::chrono::seconds keepalive { status_response_timeout / 2U };

	/**
	 * @brief automatic reconnection of mqtt client option
	*/
	static constexpr bool automatic_reconnect { true };

	/**
	 * @brief max time that the mqtt client will wait for a connection before failing;
	 *        value reasoning: TCP timeout for retransmission when TCP packet is dropped is 200ms,
	 *        this value is multiple of three of this value
	*/
	static constexpr std::chrono::milliseconds connect_timeout { 600 };

	/**
	 * @brief max messages that can be in the process of transmission simultaneously;
	 *        value reasoning: How many MQTT inflight messages can be open at one time.
	 *        The value is chosen as a recommendation from a MQTT community.
	*/
	static constexpr size_t max_inflight { 20 };
};

/**
 * @brief Constant string views
 */
class Constants {
public:
	inline static constexpr std::string_view CONFIG_PATH { "config-path" };

	inline static constexpr std::string_view LOGGING { "logging" };
	inline static constexpr std::string_view LOGGING_CONSOLE { "console" };
	inline static constexpr std::string_view LOGGING_FILE { "file" };
	inline static constexpr std::string_view LOG_LEVEL { "level" };
	inline static constexpr std::string_view LOG_USE { "use" };
	inline static constexpr std::string_view LOG_PATH { "path" };

	inline static constexpr std::string_view LOG_LEVEL_DEBUG { "DEBUG" };
	inline static constexpr std::string_view LOG_LEVEL_INFO { "INFO" };
	inline static constexpr std::string_view LOG_LEVEL_WARNING { "WARNING" };
	inline static constexpr std::string_view LOG_LEVEL_ERROR { "ERROR" };
	inline static constexpr std::string_view LOG_LEVEL_CRITICAL { "CRITICAL" };
	inline static constexpr std::string_view LOG_LEVEL_INVALID { "INVALID" };

	inline static constexpr std::string_view HELP { "help" };
	inline static constexpr std::string_view PORT { "port" };

	inline static constexpr std::string_view MODULE_PATHS { "module-paths" };
	inline static constexpr std::string_view MODULE_BINARY_PATH { "module-binary-path" };

	inline static constexpr std::string_view INTERNAL_SERVER_SETTINGS { "internal-server-settings" };

	inline static constexpr std::string_view EXTERNAL_CONNECTION { "external-connection" };
	inline static constexpr std::string_view VEHICLE_NAME { "vehicle-name" };
	inline static constexpr std::string_view COMPANY { "company" };
	inline static constexpr std::string_view EXTERNAL_ENDPOINTS { "endpoints" };
	inline static constexpr std::string_view SERVER_IP { "server-ip" };
	inline static constexpr std::string_view PROTOCOL_TYPE { "protocol-type" };

	inline static constexpr std::string_view MQTT { "MQTT" };
	inline static constexpr std::string_view DUMMY { "DUMMY" };
	inline static constexpr std::string_view MQTT_SETTINGS { "mqtt-settings" };
	inline static constexpr std::string_view SSL { "ssl" };
	inline static constexpr std::string_view CA_FILE { "ca-file" };
	inline static constexpr std::string_view CLIENT_CERT { "client-cert" };
	inline static constexpr std::string_view CLIENT_KEY { "client-key" };

	inline static constexpr std::string_view MODULES { "modules" };
	inline static constexpr std::string_view AERON_CONNECTION { "aeron:ipc"};
	inline static constexpr std::string_view SEPARATOR { ":::" };
};

}
