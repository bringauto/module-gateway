#include <bringauto/settings/SettingsParser.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/common_utils/EnumUtils.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <iostream>
#include <fstream>



namespace bringauto::settings {

bool SettingsParser::parseSettings(int argc, char **argv) {
	parseCmdArguments(argc, argv);
	if(cmdArguments_.count("help") != 0 || argc == 1) {
		return false;
	}

	if(!areCmdArgumentsCorrect()) {
		throw std::invalid_argument("Cmd arguments are not correct");
	}

	fillSettings();

	if(!areSettingsCorrect()) {
		throw std::invalid_argument("Arguments are not correct.");
	}

	return true;
}

void SettingsParser::parseCmdArguments(int argc, char **argv) {
	cxxopts::Options options { "ModuleGateway", "BringAuto Fleet Protocol Module Gateway" };
	options.add_options("General")
				   ("h, " + std::string(Constants::HELP), "Print usage")
				   ("c, " + std::string(Constants::CONFIG_PATH), "Path to configuration file", cxxopts::value<std::string>())
				   ("l, " + std::string(Constants::LOG_PATH), "Path to logs", cxxopts::value<std::string>())
				   ("v, " + std::string(Constants::VERBOSE), "Print log messages into terminal");
	options.add_options("Internal Server")(std::string(Constants::PORT), "Port on which Server listens", cxxopts::value<unsigned short>());
	options.add_options("Module Handler")(std::string(Constants::MODULE_PATHS), "Paths to shared module libraries",
										  cxxopts::value < std::vector < std::string >> ());

	cmdArguments_ = options.parse(argc, argv);

	if(cmdArguments_.count("help") != 0 || argc == 1) {
		std::cout << options.help() << std::endl;
	}
}

bool SettingsParser::areCmdArgumentsCorrect() {
	bool isCorrect = true;
	std::vector <std::string> requiredParams {
			std::string(Constants::CONFIG_PATH)
	};
	std::vector <std::string> allParameters = {
			std::string(Constants::CONFIG_PATH),
			std::string(Constants::VERBOSE),
			std::string(Constants::LOG_PATH),
			std::string(Constants::PORT),
			std::string(Constants::MODULE_PATHS)
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

bool SettingsParser::areSettingsCorrect() {
	bool isCorrect = true;

	if(!std::filesystem::exists(settings_->logPath)) {
		std::cerr << "Given log path (" << settings_->logPath << ") does not exist." << std::endl;
		isCorrect = false;
	}
	if(settings_->modulePaths.empty()) {
		std::cerr << "No shared module library provided." << std::endl;
		isCorrect = false;
	}

	return isCorrect;
}

std::shared_ptr <bringauto::settings::Settings> SettingsParser::getSettings() {
	return settings_;
}

void SettingsParser::fillSettings() {
	settings_ = std::make_shared<bringauto::settings::Settings>();

	const auto configPath = cmdArguments_[std::string(Constants::CONFIG_PATH)].as<std::string>();
	std::ifstream inputFile(configPath);
	const auto file = nlohmann::json::parse(inputFile);

	fillGeneralSettings(file);
	fillInternalServerSettings(file);
	fillModulePathsSettings(file);
	fillExternalConnectionSettings(file);
}

void SettingsParser::fillGeneralSettings(const nlohmann::json &file) {
	if(cmdArguments_.count(std::string(Constants::LOG_PATH))) {
		settings_->logPath = cmdArguments_[std::string(Constants::LOG_PATH)].as<std::string>();
	} else {
		settings_->logPath = std::filesystem::path(file[std::string(Constants::GENERAL_SETTINGS)][std::string(Constants::LOG_PATH)]);
	}
	if(cmdArguments_.count(std::string(Constants::VERBOSE))) {
		settings_->verbose = cmdArguments_.count(std::string(Constants::VERBOSE)) == 1;
	} else {
		settings_->verbose = file[std::string(Constants::GENERAL_SETTINGS)][std::string(Constants::VERBOSE)];
	}
}

void SettingsParser::fillInternalServerSettings(const nlohmann::json &file) {
	if(cmdArguments_.count(std::string(Constants::PORT))) {
		settings_->port = cmdArguments_[std::string(Constants::PORT)].as<uint32_t>();
	} else {
		settings_->port = file[std::string(Constants::INTERNAL_SERVER_SETTINGS)][std::string(Constants::PORT)];
	}
}

void SettingsParser::fillModulePathsSettings(const nlohmann::json &file) {
	if(cmdArguments_.count(std::string(Constants::MODULE_PATHS))) {
		settings_->modulePaths = cmdArguments_[std::string(Constants::MODULE_PATHS)].as < std::map < int, std::string >> ();
	} else {
		for(auto &[key, val]: file[std::string(Constants::MODULE_PATHS)].items()) {
			settings_->modulePaths[stoi(key)] = val;
		}
	}
}

void SettingsParser::fillExternalConnectionSettings(const nlohmann::json &file) {
	if(cmdArguments_.count(std::string(Constants::VEHICLE_NAME))) {
		settings_->vehicleName = cmdArguments_[std::string(Constants::VEHICLE_NAME)].as<std::string>();
	} else {
		settings_->vehicleName = file[std::string(Constants::EXTERNAL_CONNECTION)][std::string(Constants::VEHICLE_NAME)];
	}
	if(cmdArguments_.count(std::string(Constants::COMPANY))) {
		settings_->company = cmdArguments_[std::string(Constants::COMPANY)].as<std::string>();
	} else {
		settings_->company = file[std::string(Constants::EXTERNAL_CONNECTION)][std::string(Constants::COMPANY)];
	}

	for(const auto &endpoint: file[std::string(Constants::EXTERNAL_CONNECTION)][std::string(Constants::EXTERNAL_ENDPOINTS)]) {
		structures::ExternalConnectionSettings externalConnectionSettings;
		externalConnectionSettings.serverIp = endpoint[std::string(Constants::SERVER_IP)];
		externalConnectionSettings.port = endpoint[std::string(Constants::PORT)];
		externalConnectionSettings.modules = endpoint[std::string(Constants::MODULES)].get < std::vector < int >> ();

		externalConnectionSettings.protocolType = common_utils::EnumUtils::stringToProtocolType(
				endpoint[std::string(Constants::PROTOCOL_TYPE)]);
		std::string settingsName {};
		switch(externalConnectionSettings.protocolType) {
			case structures::ProtocolType::MQTT:
				settingsName = std::string(Constants::MQTT_SETTINGS);
				break;
			case structures::ProtocolType::INVALID:
			default:
				std::cerr << "Invalid protocol type: " << endpoint[std::string(Constants::PROTOCOL_TYPE)] << std::endl;
				continue;
		}
		if(endpoint.find(settingsName) != endpoint.end()) {
			for(auto &[key, val]: endpoint[settingsName].items()) {
				externalConnectionSettings.protocolSettings[key] = to_string(val);
			}
		}

		settings_->externalConnectionSettingsList.push_back(externalConnectionSettings);
	}
}

}

