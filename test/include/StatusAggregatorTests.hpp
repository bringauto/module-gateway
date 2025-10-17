#pragma once

#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/settings/LoggerId.hpp>
#include <libbringauto_logger/bringauto/logging/Logger.hpp>
#include <libbringauto_logger/bringauto/logging/ConsoleSink.hpp>
#include <bringauto/modules/IModuleManagerLibraryHandler.hpp>

#include <gtest/gtest.h>

#include <memory>



class StatusAggregatorTests: public ::testing::Test {
protected:
	void SetUp() override;

	void TearDown() override;

	static void SetUpTestSuite() {
		bringauto::settings::Logger::destroy();
		bringauto::settings::Logger::addSink<bringauto::logging::ConsoleSink>();
		bringauto::logging::LoggerSettings settings {
			"StatusAggregatorTests",
			bringauto::settings::toLoggerVerbosity(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY)
		};
		bringauto::settings::Logger::init(settings);
	}


	bringauto::modules::Buffer init_status_buffer();

	bringauto::modules::Buffer init_command_buffer();

	bringauto::modules::Buffer init_empty_buffer();

	void add_status_to_aggregator();

	void remove_device_from_status_aggregator();

	std::shared_ptr<bringauto::structures::GlobalContext> context_ {};

	std::unique_ptr <bringauto::modules::StatusAggregator> statusAggregator_ {};

	std::shared_ptr<bringauto::modules::IModuleManagerLibraryHandler> libHandler_ {};

#ifdef DEBUG
	static constexpr const char* PATH_TO_MODULE { "./test/lib/example-module/libexample-module-gateway-sharedd.so" };
#else
	static constexpr const char* PATH_TO_MODULE { "./test/lib/example-module/libexample-module-gateway-shared.so" };
#endif
	const std::string WRONG_PATH_TO_MODULE { "./bad_path.so" };
	const unsigned int MODULE = 1000;
	const unsigned int SUPPORTED_DEVICE_TYPE = 0;
	const unsigned int UNSUPPORTED_DEVICE_TYPE = 1000;
	const char *UNIQUE_DEVICE { "1000/0/button/name" };
	const char *UNIQUE_DEVICES { "1000/0/button/name,1000/0/button2/green" };
	const char *BUTTON_PRESSED { "{\"pressed\": true}" };
	const char *BUTTON_UNPRESSED = "{\"pressed\": false}";
	const char *LIT_UP = "{\"lit_up\": true}";
	const char *LIT_DOWN = "{\"lit_up\": false}";
	const char *DEVICE_ROLE = "button";
	const char *DEVICE_NAME = "name";
	const char *DEVICE_ROLE_2 = "button2";
	const char *DEVICE_NAME_2 = "green";
};
