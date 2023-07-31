#pragma once

#include <string>



namespace bringauto::settings {
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

static const std::string CONFIG_PATH { "config-path" };
static const std::string VERBOSE { "verbose" };
static const std::string LOG_PATH { "log-path" };
static const std::string HELP { "help" };
static const std::string PORT { "port" };

static const std::string MODULE_PATHS { "module-paths" };

static const std::string GENERAL_SETTINGS { "general-settings" };
static const std::string INTERNAL_SERVER_SETTINGS { "internal-server-settings" };

static const std::string EXTERNAL_CONNECTION { "external-connection" };
static const std::string VEHICLE_NAME { "vehicle-name" };
static const std::string COMPANY { "company" };
static const std::string EXTERNAL_ENDPOINTS { "endpoints" };
static const std::string SERVER_IP { "server-ip" };
static const std::string PROTOCOL_TYPE { "protocol-type" };

static const std::string MQTT_SETTINGS { "mqtt-settings" };
static const std::string SSL = "ssl";
static const std::string CA_FILE = "ca-file";
static const std::string CLIENT_CERT = "client-cert";
static const std::string CLIENT_KEY = "client-key";


static const std::string MODULES { "modules" };
}