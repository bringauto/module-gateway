#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <filesystem>
#include <vector>
#include <string>
#include <map>



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
	 * @brief path to folder, where logs will be generated
	 */
	std::filesystem::path logPath;
	/**
	 * @brief verbose switch, if true, logs will be also printed to console
	 */
	bool verbose;

	/**
	 * @brief company name for external connection
	 */
	std::string company;

	/**
	 * @brief vehicle name used in external connection
	 */
	std::string vehicleName;

	/**
	 * @brief paths to shared module libraries
	 */
	std::map<int, std::string> modulePaths;

	/**
	 * @brief Setting of external connection endpoints and protocols
	 */
	std::vector<structures::ExternalConnectionSettings> externalConnectionSettingsList;

};
}
