#pragma once

#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>
#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/aeron_communication/AeronClient.hpp>

#include <memory>



namespace bringauto::structures {

/**
 * @brief Library with library handlers and status aggregators
 */
struct ModuleLibrary {
	ModuleLibrary();

	~ModuleLibrary();

	/**
	 * @brief Load libraries from paths
	 *
	 * @param libPaths paths to the libraries
	 */
	void loadLibraries(const std::unordered_map<int, std::string> &libPaths);

	/**
	 * @brief Initialize status aggregators with context
	 *
	 * @param context global context
	 */
	void initStatusAggregators(std::shared_ptr<GlobalContext> &context);
	/// Map of module handlers, key is module id
	std::unordered_map<unsigned int, std::shared_ptr<modules::ModuleManagerLibraryHandler>> moduleLibraryHandlers {};
	/// Map of status aggregators, key is module id
	std::unordered_map<unsigned int, std::shared_ptr<modules::StatusAggregator>> statusAggregators {};
	/// Aeron client used for communication with modules
	std::shared_ptr<aeron_communication::AeronClient> aeronClient {nullptr};
};

}
