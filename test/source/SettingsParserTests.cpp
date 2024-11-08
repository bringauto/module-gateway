#include <SettingsParserTests.hpp>
#include <bringauto/common_utils/EnumUtils.hpp>

#include <gtest/gtest.h>

namespace bacu = bringauto::common_utils;


/**
 * @brief Test if settings are correctly parsed from the config file
 */
TEST_F(SettingsParserTests, ValidConfig) {
	testing_utils::ConfigMock::Config config {};
	auto result = parseConfig(config);
	EXPECT_TRUE(result);

	auto settings = settingsParser.getSettings();
	EXPECT_EQ(settings->port, config.internal_server_settings.port);
	EXPECT_EQ(settings->modulePaths, config.module_paths);

	auto logging = config.logging;
	EXPECT_EQ(bacu::EnumUtils::loggerVerbosityToString(settings->loggingSettings.console.level), logging.console.level);
	EXPECT_EQ(settings->loggingSettings.console.use, logging.console.use);
	EXPECT_EQ(bacu::EnumUtils::loggerVerbosityToString(settings->loggingSettings.file.level), logging.file.level);
	EXPECT_EQ(settings->loggingSettings.file.use, logging.file.use);
	EXPECT_EQ(settings->loggingSettings.file.path, logging.file.path);

	EXPECT_EQ(settings->company, config.external_connection.company);
	EXPECT_EQ(settings->vehicleName, config.external_connection.vehicle_name);

	auto endpoint = config.external_connection.endpoint;
	auto externalConnectionSettings = settings->externalConnectionSettingsList.front();
	EXPECT_EQ(externalConnectionSettings.protocolType, bacu::EnumUtils::stringToProtocolType(endpoint.protocol_type));
	EXPECT_EQ(externalConnectionSettings.serverIp, endpoint.server_ip);
	EXPECT_EQ(externalConnectionSettings.port, endpoint.port);
	EXPECT_EQ(externalConnectionSettings.modules, endpoint.modules);
	EXPECT_EQ(externalConnectionSettings.protocolSettings, endpoint.mqtt_settings);
}


/**
 * @brief Test if settings are correctly parsed from command line arguments
 */
TEST_F(SettingsParserTests, CmdOptionsPriority){
	testing_utils::ConfigMock::Config config {};
	auto result = parseConfig(config, {
		"--port=2000"
	});
	EXPECT_TRUE(result);

	auto settings = settingsParser.getSettings();
	EXPECT_EQ(settings->port, 2000);
}


/**
 * @brief Test if the help command option is correctly parsed
 */
TEST_F(SettingsParserTests, HelpCmdOption){
	auto result = parseConfig(testing_utils::ConfigMock::Config(), {"--help"});
	EXPECT_FALSE(result);
}


/**
 * @brief Test if duplicate command line options are correctly handled
 */
TEST_F(SettingsParserTests, DuplicateCmdOption){
	bool failed = false;
	try	{
		parseConfig(testing_utils::ConfigMock::Config(), {"--config-path=/path"});
	}catch (std::invalid_argument &e){
		EXPECT_STREQ(e.what(), "Cmd arguments are not correct.");
		failed = true;
	}
	EXPECT_TRUE(failed);
}


/**
 * @brief Test if unmatched command line options are correctly handled
 */
TEST_F(SettingsParserTests, UnmatchedCmdOption){
	bool failed = false;
	try	{
		parseConfig(testing_utils::ConfigMock::Config(), {"--made-up-option=value"});
	}catch (std::invalid_argument &e){
		EXPECT_STREQ(e.what(), "Cmd arguments are not correct.");
		failed = true;
	}
	EXPECT_TRUE(failed);
}


/**
 * @brief Test if missing required command line options are correctly handled
 */
TEST_F(SettingsParserTests, MissingRequiredConfigOption){
	bool failed = false;
	try	{
		parseConfig(testing_utils::ConfigMock::Config(), {"modulegateway_tests", "--port=2000"}, true);
	}catch (std::invalid_argument &e){
		EXPECT_STREQ(e.what(), "Cmd arguments are not correct.");
		failed = true;
	}
	EXPECT_TRUE(failed);
}


/**
 * @brief Test if non existent log path is correctly handled
 */
TEST_F(SettingsParserTests, LogPathNonExistent){
	testing_utils::ConfigMock::Config config {};
	config.logging.file.path = "/non/existent/path";
	bool failed = false;
	try {
		parseConfig(config);
	}catch (std::invalid_argument &e){
		EXPECT_STREQ(e.what(), "Arguments are not correct.");
		failed = true;
	}
	EXPECT_TRUE(failed);
}


/**
 * @brief Test if empty module paths are correctly handled 
 */
TEST_F(SettingsParserTests, EmptyModulePaths){
	testing_utils::ConfigMock::Config config {};
	config.module_paths = {};
	bool failed = false;
	try {
		parseConfig(config);
	}catch (std::invalid_argument &e){
		EXPECT_STREQ(e.what(), "Arguments are not correct.");
		failed = true;
	}
	EXPECT_TRUE(failed);
}


/**
 * @brief Test if invalid company name is correctly handled 
 */
TEST_F(SettingsParserTests, InvalidCompany){
	testing_utils::ConfigMock::Config config {};
	config.external_connection.company = "Company Name";
	bool failed = false;
	try {
		parseConfig(config);
	}catch (std::invalid_argument &e){
		EXPECT_STREQ(e.what(), "Arguments are not correct.");
		failed = true;
	}
	EXPECT_TRUE(failed);
}


/**
 * @brief Test if invalid vehicle name is correctly handled 
 */
TEST_F(SettingsParserTests, InvalidVehicleName){
	testing_utils::ConfigMock::Config config {};
	config.external_connection.vehicle_name = "Vehicle Name";
	bool failed = false;
	try {
		parseConfig(config);
	}catch (std::invalid_argument &e){
		EXPECT_STREQ(e.what(), "Arguments are not correct.");
		failed = true;
	}
	EXPECT_TRUE(failed);
}


/**
 * @brief Test if invalid protocol type is correctly handled 
 */
TEST_F(SettingsParserTests, InvalidProtocol){
	testing_utils::ConfigMock::Config config {};
	config.external_connection.endpoint.protocol_type = "INVALID";
	auto result = parseConfig(config);
	EXPECT_TRUE(result);
	EXPECT_TRUE(settingsParser.getSettings()->externalConnectionSettingsList.empty());
}
