#pragma once

#include <testing_utils/TestHandler.hpp>
#include <bringauto/settings/LoggerId.hpp>
#include <libbringauto_logger/bringauto/logging/Logger.hpp>
#include <libbringauto_logger/bringauto/logging/FileSink.hpp>
#include <libbringauto_logger/bringauto/logging/ConsoleSink.hpp>

#include <string>



class InternalServerTests: public ::testing::Test {
protected:
	const InternalProtocol::Device_Module defaultModule { InternalProtocol::Device_Module_MISSION_MODULE };
	const size_t defaultType { 1 };
	const std::string defaultRole { "TestRole" };
	const std::string defaultName { "TestName" };
	const size_t defaultPriority { 0 };
	const std::string defaultData { "Tested Data" };

	void initLogger() {
		bringauto::settings::Logger::destroy();
		bringauto::settings::Logger::addSink<bringauto::logging::ConsoleSink>();
		bringauto::logging::LoggerSettings params {
			"InternalServerTests",
			bringauto::logging::LoggerVerbosity::Debug
		};
		bringauto::settings::Logger::init(params);
	}
	void SetUp() override {
		initLogger();
	}

	void TearDown() override {
		bringauto::settings::Logger::destroy();
	}
};
