#include <bringauto/utils/utils.hpp>

#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/logging/Logger.hpp>

#include <libbringauto_logger/bringauto/logging/Logger.hpp>
#include <libbringauto_logger/bringauto/logging/FileSink.hpp>
#include <libbringauto_logger/bringauto/logging/ConsoleSink.hpp>
#include "bringauto/structures/DeviceIdentification.hpp"



namespace bringauto::utils {

void initModule(std::shared_ptr <bringauto::structures::GlobalContext> &context,
				const std::shared_ptr <bringauto::modules::ModuleManagerLibraryHandler> &libraryHandler) {
	auto status_agg = std::make_shared<bringauto::modules::StatusAggregator>(libraryHandler);
	status_agg->init_status_aggregator();
	auto moduleNumber = status_agg->get_module_number();
	bringauto::logging::Logger::logInfo("Module with number: {} started", moduleNumber);
	context->statusAggregators.emplace(moduleNumber, status_agg);
}

void initStatusAggregators(std::shared_ptr <bringauto::structures::GlobalContext> &context) {
	for(auto const &[key, path]: context->moduleLibraries) {
		initModule(context, path);
	}
}

void loadLibraries(std::map<unsigned int, std::shared_ptr<bringauto::modules::ModuleManagerLibraryHandler>> &modules,
				   const std::map<int, std::string> &libPaths) {
	for(auto const &[key, path]: libPaths) {
		modules.emplace(key, std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>());
		if(modules[key]->loadLibrary(path) != OK) {
			throw std::runtime_error("Unable to load library " + path);
		}
	}
}

void initLogger(const std::string &logPath, bool verbose) {
	using namespace bringauto::logging;
	if(verbose) {
		Logger::addSink<bringauto::logging::ConsoleSink>();
	}
	FileSink::Params paramFileSink { logPath, "ModuleGateway.log" };
	paramFileSink.maxFileSize = 50_MiB;
	paramFileSink.numberOfRotatedFiles = 5;
	paramFileSink.verbosity = Logger::Verbosity::Info;

	Logger::addSink<bringauto::logging::FileSink>(paramFileSink);
	Logger::LoggerSettings params { "ModuleGateway",
									Logger::Verbosity::Debug }; // TODO change to Info
	Logger::init(params);
}

std::vector <std::string> splitString(const std::string &input, char delimiter) {
	std::vector <std::string> tokens;
	std::istringstream iss(input);
	std::string token;

	while(std::getline(iss, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

std::string getId(const ::device_identification &device) {
	std::stringstream ss;
	ss << device.module << "/" << device.device_type << "/" << std::string{static_cast<char *>(device.device_role.data), device.device_role.size_in_bytes} << "/"
	   << std::string{static_cast<char *>(device.device_name.data), device.device_name.size_in_bytes}; // TODO we need to be able to get priority
	return ss.str();
}

}