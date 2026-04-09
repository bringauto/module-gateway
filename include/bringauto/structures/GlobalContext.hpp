#pragma once

#include <boost/asio.hpp>
#include <bringauto/settings/Settings.hpp>
#include <utility>



namespace bringauto::structures {
/**
 * @brief Context structure which is shared across ModuleGateway project.
 */
struct GlobalContext {
	GlobalContext() = default;
	explicit GlobalContext(settings::Settings settings_): settings(std::move(settings_)) {}

	/**
	 * @brief io_context shared across Module Gateway
	 */
	boost::asio::io_context ioContext {};

	/**
	 * @brief settings used in the project
	 */
	settings::Settings settings {};
};
}
