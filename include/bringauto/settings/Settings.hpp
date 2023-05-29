#pragma once

#include <filesystem>
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
	 * @brief path to folder, where logs will be generated
	 */
	std::filesystem::path logPath;
	/**
	 * @brief verbose switch, if true, logs will be also printed to console
	 */
	bool verbose;

	/**
	 * @brief paths to shared module libraries
	 */
	std::vector<std::string> modulePaths;


};
}
