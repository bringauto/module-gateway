#include <bringauto/structures/ModuleLibrary.hpp>

#include <bringauto/logging/Logger.hpp>

#include <iostream>



namespace bringauto::structures {

ModuleLibrary::~ModuleLibrary() {
	std::for_each(statusAggregators.cbegin(), statusAggregators.cend(),
				  [](auto &pair) { pair.second->destroy_status_aggregator(); });
}

void ModuleLibrary::loadLibraries(const std::map<int, std::string> &libPaths) {
	for(auto const &[key, path]: libPaths) {
		auto handler = std::make_shared<modules::ModuleManagerLibraryHandler>();
		handler->loadLibrary(path);
		if(handler->getModuleNumber() != key) {
			logging::Logger::logError("Module number from shared library {} does not match the module number from config. Config: {}, binary: {}.", path, key, handler->getModuleNumber());
			throw std::runtime_error {"Module numbers from config are not corresponding to binaries. Unable to continue. Fix configuration file."};
		}
		moduleLibraryHandlers.emplace(key, handler);
	}
}

void ModuleLibrary::initStatusAggregators(std::shared_ptr<GlobalContext> &context) {
	for(auto const &[key, libraryHandler]: moduleLibraryHandlers) {
		auto statusAggregator = std::make_shared<modules::StatusAggregator>(context, libraryHandler);
		statusAggregator->init_status_aggregator();
		auto moduleNumber = statusAggregator->get_module_number();
		if(statusAggregators.find(moduleNumber) != statusAggregators.end()) {
			logging::Logger::logWarning("Module with number: {} is already initialized, so skipping this module",
										moduleNumber);
			continue;
		}

		bool found = false;
		for(const auto &connection: context->settings->externalConnectionSettingsList) {
			const auto &modules = connection.modules;
			if(std::find(modules.cbegin(), modules.cend(), moduleNumber) != modules.cend()) {
				statusAggregators.emplace(moduleNumber, statusAggregator);
				logging::Logger::logInfo("Module with number: {} started", moduleNumber);
				found = true;
				break;
			}
		}
		if(not found) {
			logging::Logger::logWarning("Module with number: {} does not have endpoint, so skipping this module",
										moduleNumber);
		}
	}
}

}
