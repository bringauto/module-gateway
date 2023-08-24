#pragma once

#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>
#include <bringauto/modules/StatusAggregator.hpp>

#include <map>
#include <memory>



namespace bringauto::structures {

/**
 * @brief Library with library handlers and status aggregators
 */
struct ModuleLibrary {

	~ModuleLibrary();

	/**
	 * @brief Load libraries from paths
	 *
	 * @param libPaths paths to the libraries
	 */
	void loadLibraries(const std::map<int, std::string> &libPaths);

	/**
	 * @brief Initialize status aggregators with context
	 *
	 * @param context global context
	 */
	void initStatusAggregators(std::shared_ptr<bringauto::structures::GlobalContext> &context);

	std::map<unsigned int, std::shared_ptr<modules::ModuleManagerLibraryHandler>> moduleLibraryHandlers {};

	std::map<unsigned int, std::shared_ptr<modules::StatusAggregator>> statusAggregators {};
};

}