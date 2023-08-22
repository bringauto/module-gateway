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
		moduleLibraryHandlers.emplace(key, std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>());
		if(moduleLibraryHandlers[key]->loadLibrary(path) != OK) {
			throw std::runtime_error("Unable to load library " + path);
		}
	}
}

void ModuleLibrary::initStatusAggregators(std::shared_ptr<bringauto::structures::GlobalContext> &context) {
	for(auto const &[key, libraryHandler]: moduleLibraryHandlers) {
		auto statusAgregator = std::make_shared<bringauto::modules::StatusAggregator>(context, libraryHandler);
		statusAgregator->init_status_aggregator();
		auto moduleNumber = statusAgregator->get_module_number();
		bringauto::logging::Logger::logInfo("Module with number: {} started", moduleNumber);
		statusAggregators.emplace(moduleNumber, statusAgregator);
	}
}

}