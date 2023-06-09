#pragma once

#include <bringauto/settings/Settings.hpp>
#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <boost/asio.hpp>



namespace bringauto::structures {
/**
 * @brief Context structure which is shared across ModuleGateway project.
 */
struct GlobalContext {

	GlobalContext() {}

    GlobalContext(const std::shared_ptr<bringauto::settings::Settings> &settings_) : settings(settings_) {}
	/**
	 * @brief io_context shared across Module Gateway
	 */
	boost::asio::io_context ioContext;

	std::map<int, std::shared_ptr<modules::ModuleManagerLibraryHandler>> moduleLibraries {};

	/**
	 * @brief settings used in the project
	 */
	std::shared_ptr<bringauto::settings::Settings> settings {};
};
}
