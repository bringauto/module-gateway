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
		bringauto::logging::Logger::destroy();
		bringauto::logging::Logger::addSink<bringauto::logging::ConsoleSink>();
		bringauto::logging::Logger::LoggerSettings settings { 
			"ErrorAggregatorTests",
			bringauto::logging::Logger::Verbosity::Critical
		};
		bringauto::logging::Logger::init(settings);
	}

	bringauto::modules::Buffer init_status_buffer();

    bringauto::external_client::ErrorAggregator errorAggregator_ {};
	std::shared_ptr<bringauto::modules::ModuleManagerLibraryHandler> libHandler_ {};
#ifdef DEBUG
	static constexpr const char* PATH_TO_MODULE { "./test/lib/example-module/libexample-module-gateway-sharedd.so" };
#else
    static constexpr const char* PATH_TO_MODULE { "./test/lib/example-module/libexample-module-gateway-shared.so" };
#endif
    static constexpr const int MODULE = 1000;
    const unsigned int SUPPORTED_DEVICE_TYPE = 0;
	const unsigned int UNSUPPORTED_DEVICE_TYPE = 1000;
    const char *BUTTON_UNPRESSED = "{\"pressed\": false}";
};