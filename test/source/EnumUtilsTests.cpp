#include <bringauto/common_utils/EnumUtils.hpp>
#include <bringauto/settings/Constants.hpp>

#include <gtest/gtest.h>


/**
 * @brief Test EnumUtils class
 */
TEST(EnumUtilsTests, EnumUtils){
	namespace bacu = bringauto::common_utils;
	namespace bas = bringauto::structures;
	namespace baset = bringauto::settings;
	namespace balog = bringauto::logging;

	EXPECT_EQ(bacu::EnumUtils::stringToProtocolType(std::string(baset::Constants::MQTT)), bas::ProtocolType::MQTT);
	EXPECT_EQ(bacu::EnumUtils::stringToProtocolType(std::string(baset::Constants::DUMMY)), bas::ProtocolType::DUMMY);
	EXPECT_EQ(bacu::EnumUtils::stringToProtocolType("INVALID"), bas::ProtocolType::INVALID);

	EXPECT_EQ(bacu::EnumUtils::protocolTypeToString(bas::ProtocolType::MQTT), "MQTT");
	EXPECT_EQ(bacu::EnumUtils::protocolTypeToString(bas::ProtocolType::DUMMY), "DUMMY");
	EXPECT_EQ(bacu::EnumUtils::protocolTypeToString(bas::ProtocolType::INVALID), "");

	EXPECT_EQ(bacu::EnumUtils::stringToLoggerVerbosity(std::string(baset::Constants::LOG_LEVEL_DEBUG)), balog::LoggerVerbosity::Debug);
	EXPECT_EQ(bacu::EnumUtils::stringToLoggerVerbosity(std::string(baset::Constants::LOG_LEVEL_INFO)), balog::LoggerVerbosity::Info);
	EXPECT_EQ(bacu::EnumUtils::stringToLoggerVerbosity(std::string(baset::Constants::LOG_LEVEL_WARNING)), balog::LoggerVerbosity::Warning);
	EXPECT_EQ(bacu::EnumUtils::stringToLoggerVerbosity(std::string(baset::Constants::LOG_LEVEL_ERROR)), balog::LoggerVerbosity::Error);
	EXPECT_EQ(bacu::EnumUtils::stringToLoggerVerbosity(std::string(baset::Constants::LOG_LEVEL_CRITICAL)), balog::LoggerVerbosity::Critical);
	EXPECT_EQ(bacu::EnumUtils::stringToLoggerVerbosity("INVALID"), balog::LoggerVerbosity::Debug);

	EXPECT_EQ(bacu::EnumUtils::loggerVerbosityToString(balog::LoggerVerbosity::Debug), baset::Constants::LOG_LEVEL_DEBUG);
	EXPECT_EQ(bacu::EnumUtils::loggerVerbosityToString(balog::LoggerVerbosity::Info), baset::Constants::LOG_LEVEL_INFO);
	EXPECT_EQ(bacu::EnumUtils::loggerVerbosityToString(balog::LoggerVerbosity::Warning), baset::Constants::LOG_LEVEL_WARNING);
	EXPECT_EQ(bacu::EnumUtils::loggerVerbosityToString(balog::LoggerVerbosity::Error), baset::Constants::LOG_LEVEL_ERROR);
	EXPECT_EQ(bacu::EnumUtils::loggerVerbosityToString(balog::LoggerVerbosity::Critical), baset::Constants::LOG_LEVEL_CRITICAL);
}
