#include <bringauto/settings/SettingsParser.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/common_utils/EnumUtils.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <iostream>
#include <fstream>
#include <ranges>



namespace bringauto::settings {

bool SettingsParser::parseSettings(int argc, char **argv) {
	parseCmdArguments(argc, argv);
	if(cmdArguments_.count("help") != 0 || argc == 1) {
		return false;
	}

	if(!areCmdArgumentsCorrect()) {
		throw std::invalid_argument {"Cmd arguments are not correct."};
	}

	fillSettings();

	if(!areSettingsCorrect()) {
		throw std::invalid_argument {"Arguments are not correct."};
	}

	return true;
}

void SettingsParser::parseCmdArguments(int argc, char **argv) {
	cxxopts::Options options { "ModuleGateway", "BringAuto Fleet Protocol Module Gateway" };
	options.add_options("General")
				   ("h, " + std::string(Constants::HELP), "Print usage")
				   ("c, " + std::string(Constants::CONFIG_PATH), "Path to configuration file",
					cxxopts::value<std::string>());
	options.add_options("Internal Server")(std::string(Constants::PORT), "Port on which Server listens",
										   cxxopts::value<int>());

	options.allow_unrecognised_options();
	cmdArguments_ = options.parse(argc, argv);

	if(cmdArguments_.count("help") != 0 || argc == 1) {
		std::cout << options.help() << std::endl;
	}
}

bool SettingsParser::areCmdArgumentsCorrect() const {
	bool isCorrect = true;
	std::vector<std::string> requiredParams {
			std::string(Constants::CONFIG_PATH)
	};
	std::vector<std::string> allParameters = {
			std::string(Constants::CONFIG_PATH),
			std::string(Constants::PORT)
	};
	allParameters.insert(allParameters.end(), requiredParams.begin(), requiredParams.end());

	for(const auto &param: allParameters) {
		if(cmdArguments_.count(param) > 1) {
			isCorrect = false;

			std::cerr << "[ERROR] Found duplicate --" << param << " cmdline parameter!" << std::endl;
		}
	}

	if(!cmdArguments_.unmatched().empty()) {
		std::string errorMessage = "Unmatched arguments! use -h to see arguments, arguments should be in format -<short name> <value> or --<long name>=<value>, unmatched arguments: ";
		for(const auto &param: cmdArguments_.unmatched()) {
			errorMessage += param + " ";
		}
		std::cerr << "[ERROR] " << errorMessage << std::endl;
		isCorrect = false;
	}

	for(const auto &param: requiredParams) {
		if(cmdArguments_.count(param) < 1) {
			isCorrect = false;
			std::cerr << "Please provide " << param << " argument" << std::endl;
		}
	}

	return isCorrect;
}

bool SettingsParser::areSettingsCorrect() const {
	bool isCorrect = true;

	if(settings_->loggingSettings.file.use && !std::filesystem::exists(settings_->loggingSettings.file.path)) {
		std::cerr << "Given log path (" << settings_->loggingSettings.file.path << ") does not exist." << std::endl;
		isCorrect = false;
	}
	if(settings_->modulePaths.empty()) {
		std::cerr << "No shared module library provided." << std::endl;
		isCorrect = false;
	}
	if(!settings_->moduleBinaryPath.empty() && !std::filesystem::exists(settings_->moduleBinaryPath)) {
		std::cerr << "Given module binary path (" << settings_->moduleBinaryPath << ") does not exist." << std::endl;
		isCorrect = false;
	}
	if(!std::regex_match(settings_->company, std::regex("^[a-z0-9_]+$"))) {
		std::cerr << "Company name (" << settings_->company << ") is not valid." << std::endl;
		isCorrect = false;
	}
	if(!std::regex_match(settings_->vehicleName, std::regex("^[a-z0-9_]+$"))) {
		std::cerr << "Vehicle name (" << settings_->vehicleName << ") is not valid." << std::endl;
		isCorrect = false;
	}

	std::ranges::any_of(settings_->externalConnectionSettingsList, [&](auto& externalConnectionSettings){
		return std::ranges::any_of(externalConnectionSettings.modules, [&](auto const& externalModuleId) {
			bool isMissing = !settings_->modulePaths.contains(externalModuleId);
			if (isMissing)
			{
				std::cerr << "Module " << externalModuleId <<
				" is defined in external-connection endpoint modules but is not specified in module-paths" << std::endl;
				isCorrect = false;
			}
			return isMissing;
		});
	});

	return isCorrect;
}

std::shared_ptr<Settings> SettingsParser::getSettings() {
	return settings_;
}

void SettingsParser::fillSettings() {
	settings_ = std::make_shared<Settings>();

	const auto configPath = cmdArguments_[std::string(Constants::CONFIG_PATH)].as<std::string>();
	std::ifstream inputFile(configPath);
	const auto file = nlohmann::json::parse(inputFile);

	fillLoggingSettings(file);
	fillInternalServerSettings(file);
	fillModulePathsSettings(file);
	fillExternalConnectionSettings(file);
}

void SettingsParser::fillLoggingSettings(const nlohmann::json &file) const {
	const auto &consoleLogging = file[std::string(Constants::LOGGING)][std::string(Constants::LOGGING_CONSOLE)];
	const auto &fileLogging = file[std::string(Constants::LOGGING)][std::string(Constants::LOGGING_FILE)];

	settings_->loggingSettings.console.use = consoleLogging[std::string(Constants::LOG_USE)];
	settings_->loggingSettings.console.level = common_utils::EnumUtils::stringToLoggerVerbosity(
		consoleLogging[std::string(Constants::LOG_LEVEL)]);

	settings_->loggingSettings.file.use = fileLogging[std::string(Constants::LOG_USE)];
	settings_->loggingSettings.file.level = common_utils::EnumUtils::stringToLoggerVerbosity(
		fileLogging[std::string(Constants::LOG_LEVEL)]);
	settings_->loggingSettings.file.path = std::filesystem::path(fileLogging[std::string(Constants::LOG_PATH)]);
}

void SettingsParser::fillInternalServerSettings(const nlohmann::json &file) const {
	if(cmdArguments_.count(std::string(Constants::PORT))) {
		settings_->port = cmdArguments_[std::string(Constants::PORT)].as<int>();
	} else {
		settings_->port = file[std::string(Constants::INTERNAL_SERVER_SETTINGS)][std::string(Constants::PORT)];
	}
}

void SettingsParser::fillModulePathsSettings(const nlohmann::json &file) const {
	for(auto &[key, val]: file[std::string(Constants::MODULE_PATHS)].items()) {
		val.get_to(settings_->modulePaths[stoi(key)]);
	}
	file.at(std::string(Constants::MODULE_BINARY_PATH)).get_to(settings_->moduleBinaryPath);
}

void SettingsParser::fillExternalConnectionSettings(const nlohmann::json &file) const {
	file.at(std::string(Constants::EXTERNAL_CONNECTION)).at(std::string(Constants::VEHICLE_NAME)).get_to(
			settings_->vehicleName);
	file.at(std::string(Constants::EXTERNAL_CONNECTION)).at(std::string(Constants::COMPANY)).get_to(
			settings_->company);

	for(const auto &endpoint: file[std::string(Constants::EXTERNAL_CONNECTION)][std::string(
			Constants::EXTERNAL_ENDPOINTS)]) {
		structures::ExternalConnectionSettings externalConnectionSettings {};
		externalConnectionSettings.protocolType = common_utils::EnumUtils::stringToProtocolType(
				endpoint[std::string(Constants::PROTOCOL_TYPE)]);
		std::string settingsName {};
		switch(externalConnectionSettings.protocolType) {
			case structures::ProtocolType::MQTT:
				settingsName = std::string(Constants::MQTT_SETTINGS);
				break;
			case structures::ProtocolType::DUMMY:
				break;
			case structures::ProtocolType::INVALID:
			default:
				std::cerr << "Invalid protocol type: " << endpoint[std::string(Constants::PROTOCOL_TYPE)] << std::endl;
				continue;
		}

		endpoint.at(std::string(Constants::SERVER_IP)).get_to(externalConnectionSettings.serverIp);
		externalConnectionSettings.port = endpoint[std::string(Constants::PORT)];
		externalConnectionSettings.modules = endpoint[std::string(Constants::MODULES)].get<std::vector<int >>();
		
		if(!settingsName.empty() && endpoint.find(settingsName) != endpoint.end()) {
			for(auto &[key, val]: endpoint[settingsName].items()) {
				externalConnectionSettings.protocolSettings[key] = to_string(val);
			}
		}

		settings_->externalConnectionSettingsList.push_back(externalConnectionSettings);
	}
}

std::string SettingsParser::serializeToJson() const {
	nlohmann::json settingsAsJson {};

	settingsAsJson[std::string(Constants::LOGGING)][std::string(Constants::LOGGING_CONSOLE)]
		[std::string(Constants::LOG_USE)] = settings_->loggingSettings.console.use;
	settingsAsJson[std::string(Constants::LOGGING)][std::string(Constants::LOGGING_CONSOLE)][std::string(Constants::LOG_LEVEL)] =
		common_utils::EnumUtils::loggerVerbosityToString(settings_->loggingSettings.console.level);
	settingsAsJson[std::string(Constants::LOGGING)][std::string(Constants::LOGGING_FILE)]
		[std::string(Constants::LOG_USE)] = settings_->loggingSettings.file.use;
	settingsAsJson[std::string(Constants::LOGGING)][std::string(Constants::LOGGING_FILE)][std::string(Constants::LOG_LEVEL)] =
		common_utils::EnumUtils::loggerVerbosityToString(settings_->loggingSettings.file.level);
	settingsAsJson[std::string(Constants::LOGGING)][std::string(Constants::LOGGING_FILE)]
		[std::string(Constants::LOG_PATH)] = settings_->loggingSettings.file.path;

	settingsAsJson[std::string(Constants::INTERNAL_SERVER_SETTINGS)][std::string(Constants::PORT)] = settings_->port;
	for(const auto &[key, val]: settings_->modulePaths) {
		settingsAsJson[std::string(Constants::MODULE_PATHS)][std::to_string(key)] = val;
	}

	settingsAsJson[std::string(Constants::EXTERNAL_CONNECTION)][std::string(Constants::COMPANY)] = settings_->company;
	settingsAsJson[std::string(Constants::EXTERNAL_CONNECTION)][std::string(Constants::VEHICLE_NAME)] = settings_->vehicleName;
	nlohmann::json::array_t endpoints {};
	for(const auto &endpoint: settings_->externalConnectionSettingsList) {
		nlohmann::json endpointAsJson {};
		endpointAsJson[std::string(Constants::SERVER_IP)] = endpoint.serverIp;
		endpointAsJson[std::string(Constants::PORT)] = endpoint.port;
		endpointAsJson[std::string(Constants::MODULES)] = endpoint.modules;
		endpointAsJson[std::string(Constants::PROTOCOL_TYPE)] = common_utils::EnumUtils::protocolTypeToString(endpoint.protocolType);
		std::string settingsName {};
		switch (endpoint.protocolType) {
			case structures::ProtocolType::MQTT:
				settingsName = std::string(Constants::MQTT_SETTINGS);
				break;
			case structures::ProtocolType::DUMMY:
				// DUMMY has no protocol-specific settings; settingsName stays empty
				break;
			case structures::ProtocolType::INVALID:
				settingsName = "INVALID";
				break;
		}
		for(const auto &[key, val]: endpoint.protocolSettings) {
			endpointAsJson[settingsName][key] = nlohmann::json::parse(val);
		}
		endpoints.push_back(endpointAsJson);
	}
	settingsAsJson[std::string(Constants::EXTERNAL_CONNECTION)][std::string(Constants::EXTERNAL_ENDPOINTS)] = endpoints;
	return settingsAsJson.dump(4);
}

}
