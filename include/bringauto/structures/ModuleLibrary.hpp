#pragma once

#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>
#include <bringauto/modules/StatusAggregator.hpp>

#include <map>
#include <memory>



namespace bringauto::structures {

struct ModuleLibrary {

	~ModuleLibrary();

	void loadLibraries(const std::map<int, std::string> &libPaths);

	void initStatusAggregators(std::shared_ptr <bringauto::structures::GlobalContext> &context);

	std::map<unsigned int, std::shared_ptr<modules::ModuleManagerLibraryHandler>> moduleLibraryHandlers {};

	std::map<unsigned int, std::shared_ptr<modules::StatusAggregator>> statusAggregators {};
};

}