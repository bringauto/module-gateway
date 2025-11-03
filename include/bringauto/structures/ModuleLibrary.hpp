#pragma once

#include <bringauto/modules/IModuleManagerLibraryHandler.hpp>
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
	void loadLibraries(const std::unordered_map<int, std::filesystem::path> &libPaths);

	/**
	 * @brief Load libraries from paths
	 *
	 * @param libPaths paths to the libraries
	 * @param moduleBinaryPath path to module binary for async function execution over shared memory
	 */
	void loadLibraries(const std::unordered_map<int, std::filesystem::path> &libPaths, const std::filesystem::path &moduleBinaryPath);

	/**
	 * @brief Initialize status aggregators with context
	 *
	 * @param context global context
	 */
	void initStatusAggregators(std::shared_ptr<GlobalContext> &context);
	/// Map of module handlers, key is module id
	std::unordered_map<unsigned int, std::shared_ptr<modules::IModuleManagerLibraryHandler>> moduleLibraryHandlers {};
	/// Map of status aggregators, key is module id
	std::unordered_map<unsigned int, std::shared_ptr<modules::StatusAggregator>> statusAggregators {};
};

}
