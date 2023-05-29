#include <bringauto/settings/SettingsParser.hpp>
#include <bringauto/settings/Constants.hpp>

#include <iostream>
#include <fstream>



namespace bringauto::settings {

bool SettingsParser::parseSettings(int argc, char **argv) {
	parseCmdArguments(argc, argv);
	if(cmdArguments_.count("help") || argc == 1) {
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
				   ("h, " + HELP, "Print usage")
				   ("c, " + CONFIG_PATH, "Path to configuration file", cxxopts::value<std::string>())
				   ("l, " + LOG_PATH, "Path to logs", cxxopts::value<std::string>())
				   ("v, " + VERBOSE, "Print log messages into terminal");
	options.add_options("Internal Server")(PORT, "Port on which Server listens", cxxopts::value<unsigned short>());
	options.add_options("Module Handler")(MODULE_PATHS, "Paths to shared module libraries",
										  cxxopts::value<std::vector<std::string>>());

	cmdArguments_ = options.parse(argc, argv);

	if(cmdArguments_.count("help") || argc == 1) {
		std::cout << options.help() << std::endl;
	}
}

bool SettingsParser::areCmdArgumentsCorrect() {
	bool isCorrect = true;
	std::vector<std::string> requiredParams {
			CONFIG_PATH
	};
	std::vector<std::string> allParameters = {
			CONFIG_PATH,
			VERBOSE,
			LOG_PATH,
			PORT,
			MODULE_PATHS
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

std::shared_ptr<bringauto::settings::Settings> SettingsParser::getSettings() {
	return settings_;
}

void SettingsParser::fillSettings() {
	settings_ = std::make_shared<bringauto::settings::Settings>();

	const auto configPath = cmdArguments_[CONFIG_PATH].as<std::string>();
	std::ifstream inputFile(configPath);
	const auto file = nlohmann::json::parse(inputFile);

	fillGeneralSettings(file);
	fillInternalServerSettings(file);
	fillModuleHandlerSettings(file);
}

void SettingsParser::fillGeneralSettings(const nlohmann::json &file) {
	if(cmdArguments_.count(LOG_PATH)) {
		settings_->logPath = cmdArguments_[LOG_PATH].as<std::string>();
	} else {
		settings_->logPath = std::filesystem::path(file[GENERAL_SETTINGS][LOG_PATH]);
	}
	if(cmdArguments_.count(VERBOSE)) {
		settings_->verbose = cmdArguments_.count(VERBOSE) == 1;
	} else {
		settings_->verbose = file[GENERAL_SETTINGS][VERBOSE];
	}
}

void SettingsParser::fillInternalServerSettings(const nlohmann::json &file) {
	if(cmdArguments_.count(PORT)) {
		settings_->port = cmdArguments_[PORT].as<uint32_t>();
	} else {
		settings_->port = file[INTERNAL_SERVER_SETTINGS][PORT];
	}
}

void SettingsParser::fillModuleHandlerSettings(const nlohmann::json &file) {
	if(cmdArguments_.count(MODULE_PATHS)) {
		settings_->modulePaths = cmdArguments_[MODULE_PATHS].as<std::vector<std::string>>();
	} else {
		settings_->modulePaths = std::vector<std::string>(file[MODULE_HANDLER_SETTINGS][MODULE_PATHS]);
	}
}

}

