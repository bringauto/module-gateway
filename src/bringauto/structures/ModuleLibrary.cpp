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
		moduleLibraryHandlers.emplace(key, std::make_shared<modules::ModuleManagerLibraryHandler>());
		moduleLibraryHandlers[key]->loadLibrary(path);
	}
}

void ModuleLibrary::initStatusAggregators(std::shared_ptr<structures::GlobalContext> &context) {
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
			logging::Logger::logWarning("Module with number: {} does now have endpoint, so skipping this module",
										moduleNumber);
		}
	}
}

}