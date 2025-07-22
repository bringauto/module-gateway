#pragma once

#include <bringauto/settings/LoggerWrapper.hpp>

#ifndef MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY
#define MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY 0 // DEBUG
#endif



namespace bringauto::settings {

static_assert(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY >= 0 && MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY <= 4,
			  "Invalid MINIMUM_LOGGER_VERBOSITY defined. Use DEBUG(0), INFO(1), WARNING(2), ERROR(3), or CRITICAL(4).");

/**
 * @brief Converts the minimum logger verbosity defined in CMake to LoggerVerbosity enum.
 * @param verbosityNumber The minimum logger verbosity defined in CMake.
 */
constexpr logging::LoggerVerbosity toLoggerVerbosity(uint8_t verbosityNumber) {
	switch (verbosityNumber) {
		case 1:
			return logging::LoggerVerbosity::Info;
		case 2:
			return logging::LoggerVerbosity::Warning;
		case 3:
			return logging::LoggerVerbosity::Error;
		case 4:
			return logging::LoggerVerbosity::Critical;
		case 0:
		default:
			return logging::LoggerVerbosity::Debug;
	}
}

constexpr logging::LoggerId logId = {.id = "ModuleGateway"};
using BaseLogger = logging::Logger<logId, logging::LoggerImpl>;

using Logger = LoggerWrapper<BaseLogger, toLoggerVerbosity(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY)>;
}
