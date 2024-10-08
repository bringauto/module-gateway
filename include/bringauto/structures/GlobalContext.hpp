#pragma once

#include <boost/asio.hpp>
#include <bringauto/settings/Settings.hpp>



namespace bringauto::structures {
/**
 * @brief Context structure which is shared across ModuleGateway project.
 */
struct GlobalContext {

	GlobalContext() = default;

	explicit GlobalContext(const std::shared_ptr<settings::Settings> &settings_): settings(settings_) {}

	/**
	 * @brief io_context shared across Module Gateway
	 */
	boost::asio::io_context ioContext {};

	/**
	 * @brief settings used in the project
	 */
	std::shared_ptr<settings::Settings> settings {};
};
}
