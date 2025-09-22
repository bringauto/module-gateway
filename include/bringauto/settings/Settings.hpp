#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <bringauto/structures/LoggingSettings.hpp>

#include <vector>
#include <string>



namespace bringauto::settings {
/**
 * Global settings structure
 */
struct Settings {
	/**
 	 * @brief port on which the server is started
 	 */
	unsigned short port;

	/**
	 * @brief company name for external connection
	 */
	std::string company {};

	/**
	 * @brief vehicle name used in external connection
	 */
	std::string vehicleName {};

	/**
	 * @brief paths to shared module libraries
	 */
	std::unordered_map<int, std::string> modulePaths {};

	/**
	 * @brief path to module binary
	 */
	std::string moduleBinaryPath {};

	/**
	 * @brief Setting of external connection endpoints and protocols
	 */
	std::vector<structures::ExternalConnectionSettings> externalConnectionSettingsList {};

	/**
	 * @brief Settings for logging
	 */
	structures::LoggingSettings loggingSettings {};
};
}
