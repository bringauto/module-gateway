#pragma once

#include <string>
#include <chrono>



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

struct Constants {
	/// constant strings for parsing command line arguments

	const static std::string CONFIG_PATH;
	const static std::string VERBOSE;
	const static std::string LOG_PATH;
	const static std::string HELP;
	const static std::string PORT;

	const static std::string MODULE_PATHS;

	const static std::string GENERAL_SETTINGS;
	const static std::string INTERNAL_SERVER_SETTINGS;

	const static std::string EXTERNAL_CONNECTION;
	const static std::string VEHICLE_NAME;
	const static std::string COMPANY;
	const static std::string EXTERNAL_ENDPOINTS;
	const static std::string SERVER_IP;
	const static std::string PROTOCOL_TYPE;

	const static std::string MQTT_SETTINGS;
	const static std::string SSL;
	const static std::string CA_FILE;
	const static std::string CLIENT_CERT;
	const static std::string CLIENT_KEY;

	const static std::string MODULES;
};

}