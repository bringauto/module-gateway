#pragma once

#include <bringauto/settings/LoggerWrapper.hpp>

#ifndef MINIMUM_LOGGER_VERBOSITY
#define MINIMUM_LOGGER_VERBOSITY 0 // DEBUG
#endif

#if MINIMUM_LOGGER_VERBOSITY == 0 // DEBUG
#define LOGGER_VERBOSITY bringauto::logging::LoggerVerbosity::Debug
#elif MINIMUM_LOGGER_VERBOSITY == 1 // INFO
#define LOGGER_VERBOSITY bringauto::logging::LoggerVerbosity::Info
#elif MINIMUM_LOGGER_VERBOSITY == 2 // WARNING
#define LOGGER_VERBOSITY bringauto::logging::LoggerVerbosity::Warning
#elif MINIMUM_LOGGER_VERBOSITY == 3 // ERROR
#define LOGGER_VERBOSITY bringauto::logging::LoggerVerbosity::Error
#elif MINIMUM_LOGGER_VERBOSITY == 4 // CRITICAL
#define LOGGER_VERBOSITY bringauto::logging::LoggerVerbosity::Critical
#else
#error "Invalid MINIMUM_LOGGER_VERBOSITY defined. Use DEBUG(0), INFO(1), WARNING(2), ERROR(3), or CRITICAL(4)."
#endif



namespace bringauto::settings {

constexpr logging::LoggerId logId = {.id = "ModuleGateway"};
using BaseLogger = logging::Logger<logId, logging::LoggerImpl>;

using Logger = LoggerWrapper<BaseLogger, LOGGER_VERBOSITY>;
}
