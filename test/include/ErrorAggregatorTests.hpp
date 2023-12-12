#pragma once

#include <bringauto/external_client/ErrorAggregator.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/logging/ConsoleSink.hpp>
#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <gtest/gtest.h>



class ErrorAggregatorTests: public ::testing::Test {
protected:
	void SetUp() override;

	void TearDown() override;

    static void SetUpTestSuite() {
		bringauto::logging::Logger::addSink<bringauto::logging::ConsoleSink>();
		bringauto::logging::Logger::LoggerSettings settings { "StatusAggregatorTests",
															  bringauto::logging::Logger::Verbosity::Critical };
		bringauto::logging::Logger::init(settings);
	}

    const struct device_identification init_device_id(unsigned int type, const char* deviceRole, const char* deviceName);

	struct buffer init_status_buffer();

    bringauto::external_client::ErrorAggregator errorAggregator;

	static constexpr const char* PATH_TO_MODULE { "./libs/button_module/libbutton_module.so" };
    const unsigned int SUPPORTED_DEVICE_TYPE = 0;
	const unsigned int UNSUPPORTED_DEVICE_TYPE = 1000;
    const char *BUTTON_UNPRESSED = "{\"pressed\": false}";
};