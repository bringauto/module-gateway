#pragma once

#include <string>
#include <chrono>



namespace bringauto::settings {

struct Constants {
	const static std::string CONFIG_PATH;
};

/**
 * @brief timeout that is defined in fleet protocol,
 * active connection between Internal Client and internal Server is disconnected and removed after this timeout
 */
constexpr std::chrono::seconds fleet_protocol_timeout_length { 30 };

/**
 * @brief timeout that is defined in fleet protocol,
 * reconnect time between External client and External server after disconnect
 */
constexpr int reconnect_delay { 30 };

/**
 * @brief time between checks of atomic queue used for one-way communication from Module Handler to Internal Server
 */
constexpr std::chrono::seconds queue_timeout_length { 3 };
/**
 * &brief Fleet Protocol defines messages as always starting with 4 bytes header
 */
constexpr uint8_t header { 4 };
/**
 * @brief maximal amount of bytes received, that can be processed in one cycle
 */
constexpr size_t buffer_length = 1024;


/// constant strings for parsing command line arguments

const std::string VERBOSE = { "verbose" };
const std::string LOG_PATH { "log-path" };
const std::string HELP { "help" };
const std::string PORT { "port" };

const std::string MODULE_PATHS { "module-paths" };

const std::string GENERAL_SETTINGS { "general-settings" };
const std::string INTERNAL_SERVER_SETTINGS { "internal-server-settings" };

const std::string EXTERNAL_CONNECTION { "external-connection" };
const std::string VEHICLE_NAME { "vehicle-name" };
const std::string COMPANY { "company" };
const std::string EXTERNAL_ENDPOINTS { "endpoints" };
const std::string SERVER_IP { "server-ip" };
const std::string PROTOCOL_TYPE { "protocol-type" };

const std::string MQTT_SETTINGS { "mqtt-settings" };
const std::string SSL = { "ssl" };
const std::string CA_FILE = { "ca-file" };
const std::string CLIENT_CERT = { "client-cert" };
const std::string CLIENT_KEY = { "client-key" };

const std::string MODULES { "modules" };
}