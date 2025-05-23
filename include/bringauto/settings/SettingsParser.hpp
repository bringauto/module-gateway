#pragma once

#include <bringauto/settings/Settings.hpp>

#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
#include <memory>



namespace bringauto::settings {

/**
 * @brief Class implementing functions for command line arguments parsing using cxxopts
 */
class SettingsParser {

public:
	/**
	 * @brief Method parses commandline arguments into Settings class
	 * Method throws exception if wrong arguments were provided, Settings can be obtained after
	 * calling this method with getSettings() method
	 * @param argc number of commandline arguments
	 * @param argv array of commandline arguments
	 * @return true if successful, false if help was printed and parameters were not provided
	 */
	bool parseSettings(int argc, char **argv);

	/**
	 * @brief Can be used after calling parseSettings(...) to get settings
	 * @return shared ptr to settings
	 */
	std::shared_ptr<Settings> getSettings();

	/**
	 * @brief Serializes settings to json
	 * @return string with json representation of settings
	 */
	std::string serializeToJson();

private:
	cxxopts::ParseResult cmdArguments_ {};
	std::shared_ptr<Settings> settings_ {};

	void parseCmdArguments(int argc, char **argv);

	bool areCmdArgumentsCorrect();

	bool areSettingsCorrect();

	void fillSettings();

	void fillLoggingSettings(const nlohmann::json &file);

	void fillInternalServerSettings(const nlohmann::json &file);

	void fillModulePathsSettings(const nlohmann::json &file);

	void fillExternalConnectionSettings(const nlohmann::json &file);
};
}
