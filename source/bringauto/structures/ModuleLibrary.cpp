#include <bringauto/structures/ModuleLibrary.hpp>
#include <bringauto/modules/ModuleManagerLibraryHandlerLocal.hpp>
#include <bringauto/modules/ModuleManagerLibraryHandlerAsync.hpp>

#include <bringauto/settings/LoggerId.hpp>



namespace bringauto::structures {

ModuleLibrary::~ModuleLibrary() {
	std::for_each(statusAggregators.cbegin(), statusAggregators.cend(),
				  [](auto &pair) { pair.second->destroy_status_aggregator(); });
}

void ModuleLibrary::loadLibraries(const std::unordered_map<int, std::filesystem::path> &libPaths) {
	std::shared_ptr<modules::IModuleManagerLibraryHandler> handler;
	for(auto const &[key, path]: libPaths) {
		handler = std::make_shared<modules::ModuleManagerLibraryHandlerLocal>();
		handler->loadLibrary(path);
		if(handler->getModuleNumber() != key) {
			settings::Logger::logError("Module number from shared library {} does not match the module number from config. Config: {}, binary: {}.", path.string(), key, handler->getModuleNumber());
			throw std::runtime_error {"Module numbers from config are not corresponding to binaries. Unable to continue. Fix configuration file."};
		}
		auto [it, inserted] = moduleLibraryHandlers.try_emplace(key, handler);
		if(!inserted) {
			settings::Logger::logWarning("Module with number: {} is already registered, skipping duplicate", key);
		}
	}
}

void ModuleLibrary::loadLibraries(const std::unordered_map<int, std::filesystem::path> &libPaths, const std::filesystem::path &moduleBinaryPath) {
	std::shared_ptr<modules::IModuleManagerLibraryHandler> handler;
	for(auto const &[key, path]: libPaths) {
		handler = std::make_shared<modules::ModuleManagerLibraryHandlerAsync>(moduleBinaryPath, key);
		handler->loadLibrary(path);
		if(handler->getModuleNumber() != key) {
			settings::Logger::logError("Module number from shared library {} does not match the module number from config. Config: {}, binary: {}.", path.string(), key, handler->getModuleNumber());
			throw std::runtime_error {"Module numbers from config are not corresponding to binaries. Unable to continue. Fix configuration file."};
		}
		auto [it, inserted] = moduleLibraryHandlers.try_emplace(key, handler);
		if(!inserted) {
			settings::Logger::logWarning("Module with number: {} is already registered, skipping duplicate", key);
		}
	}
}

void ModuleLibrary::initStatusAggregators(std::shared_ptr<GlobalContext> &context) {
	for(auto const &[key, libraryHandler]: moduleLibraryHandlers) {
		auto statusAggregator = std::make_shared<modules::StatusAggregator>(context, libraryHandler);
		statusAggregator->init_status_aggregator();
		auto moduleNumber = statusAggregator->get_module_number();
		if(statusAggregators.contains(moduleNumber)) {
			settings::Logger::logWarning("Module with number: {} is already initialized, so skipping this module",
										moduleNumber);
			continue;
		}

		bool found = false;
		for(const auto &connection: context->settings->externalConnectionSettingsList) {
			const auto &modules = connection.modules;
			if(std::find(modules.cbegin(), modules.cend(), moduleNumber) != modules.cend()) {
				statusAggregators.try_emplace(moduleNumber, statusAggregator);
				settings::Logger::logInfo("Module with number: {} started", moduleNumber);
				found = true;
				break;
			}
		}
		if(not found) {
			settings::Logger::logWarning("Module with number: {} does not have endpoint, so skipping this module",
										moduleNumber);
		}
	}
}

}
