#pragma once

#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>
#include <bringauto/modules/ModuleManagerLibraryHandlerAsync.hpp>
#include <bringauto/modules/StatusAggregator.hpp>

#include <memory>



namespace bringauto::structures {

/**
 * @brief Library with library handlers and status aggregators
 */
struct ModuleLibrary {
	ModuleLibrary() = default;

	~ModuleLibrary();

	/**
	 * @brief Load libraries from paths
	 *
	 * @param libPaths paths to the libraries
	 */
	void loadLibraries(const std::unordered_map<int, std::string> &libPaths, const std::string &moduleBinaryPath);

	/**
	 * @brief Initialize status aggregators with context
	 *
	 * @param context global context
	 */
	void initStatusAggregators(std::shared_ptr<GlobalContext> &context);
	/// Map of module handlers, key is module id
	std::unordered_map<unsigned int, std::shared_ptr<modules::ModuleManagerLibraryHandlerAsync>> moduleLibraryHandlers {}; //TODO select type from config
	/// Map of status aggregators, key is module id
	std::unordered_map<unsigned int, std::shared_ptr<modules::StatusAggregator>> statusAggregators {};
};

}
