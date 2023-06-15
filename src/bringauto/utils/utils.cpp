#include <bringauto/utils/utils.hpp>

#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/logging/Logger.hpp>

#include <libbringauto_logger/bringauto/logging/Logger.hpp>
#include <libbringauto_logger/bringauto/logging/FileSink.hpp>
#include <libbringauto_logger/bringauto/logging/ConsoleSink.hpp>



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
									Logger::Verbosity::Info };
	Logger::init(params);
}

::device_identification mapToDeviceId(const InternalProtocol::Device &device) {
	return ::device_identification { .module = device.module(),
			.device_type = device.devicetype(),
			.device_role = device.devicerole().c_str(),
			.device_name = device.devicename().c_str(),
			.priority = device.priority() };
}

}