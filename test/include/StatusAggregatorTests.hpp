#pragma once

#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/logging/ConsoleSink.hpp>
#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <gtest/gtest.h>

#include <memory>



class StatusAggregatorTests: public ::testing::Test {
protected:
	void SetUp() override;

	void TearDown() override;

	static void SetUpTestSuite() {
		bringauto::logging::Logger::addSink<bringauto::logging::ConsoleSink>();
		bringauto::logging::Logger::LoggerSettings settings { "StatusAggregatorTests",
															  bringauto::logging::Logger::Verbosity::Critical };
		bringauto::logging::Logger::init(settings);
		context = std::make_shared<bringauto::structures::GlobalContext>();

		auto libHandler = std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>();
		libHandler->loadLibrary(PATH_TO_MODULE);
		statusAggregator = std::make_unique<bringauto::modules::StatusAggregator>(context, libHandler);
	}

	const struct device_identification init_device_id(unsigned int type, const char* deviceRole, const char* deviceName);

	struct buffer init_status_buffer();

	struct buffer init_command_buffer();

	void add_status_to_aggregator();

	inline static std::shared_ptr<bringauto::structures::GlobalContext> context;

	inline static std::unique_ptr <bringauto::modules::StatusAggregator> statusAggregator;

	static constexpr const char* PATH_TO_MODULE { "./libs/button_module/libbutton_module.so" };
	const std::string WRONG_PATH_TO_MODULE { "./bad_path.so" };
	const unsigned int SUPPORTED_DEVICE_TYPE = 0;
	const unsigned int UNSUPPORTED_DEVICE_TYPE = 1000;
	const char *UNIQUE_DEVICE { "2/0/button/name" };
	const char *UNIQUE_DEVICES { "2/0/button/name,2/0/button2/green" };
	const char *BUTTON_PRESSED { "{\"pressed\": true}" };
	const char *BUTTON_UNPRESSED = "{\"pressed\": false}";
	const char *LIT_UP = "{\"lit_up\": true}";
	const char *LIT_DOWN = "{\"lit_up\": false}";
	const char *DEVICE_ROLE = "button";
	const char *DEVICE_NAME = "name";
	const char *DEVICE_ROLE_2 = "button2";
	const char *DEVICE_NAME_2 = "green";
};