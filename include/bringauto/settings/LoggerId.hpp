#pragma once

#include <bringauto/settings/LoggerWrapper.hpp>

#ifndef MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY
#define MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY "DEBUG"
#endif



namespace {

/**
 * @brief Compile time string comparison
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @return true if both strings are equal, false otherwise
 */
constexpr bool stringsEqual(char const *str1, char const *str2) {
	return std::string_view(str1) == str2;
}

static_assert(stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "DEBUG") ||
			  stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "INFO") ||
			  stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "WARNING") ||
			  stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "ERROR") ||
			  stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "CRITICAL"),
			  "Invalid MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY defined. Use DEBUG, INFO, WARNING, ERROR, or CRITICAL.");

}

namespace bringauto::settings {

/**
 * @brief Converts the minimum logger verbosity defined in CMake to LoggerVerbosity enum.
 * @param verbosityString The minimum logger verbosity defined in CMake.
 */
constexpr bringauto::logging::LoggerVerbosity toLoggerVerbosity(std::string_view verbosityString) {
	if (verbosityString == "INFO") {
		return bringauto::logging::LoggerVerbosity::Info;
	} else if (verbosityString == "WARNING") {
		return bringauto::logging::LoggerVerbosity::Warning;
	} else if (verbosityString == "ERROR") {
		return bringauto::logging::LoggerVerbosity::Error;
	} else if (verbosityString == "CRITICAL") {
		return bringauto::logging::LoggerVerbosity::Critical;
	}
	return bringauto::logging::LoggerVerbosity::Debug;
}

constexpr logging::LoggerId logId = {.id = "ModuleGateway"};
using BaseLogger = logging::Logger<logId, logging::LoggerImpl>;

using Logger = LoggerWrapper<BaseLogger, toLoggerVerbosity(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY)>;

}
