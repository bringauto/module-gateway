#pragma once

#include <libbringauto_logger/bringauto/logging/Logger.hpp>



namespace bringauto::settings {
/**
 * @brief Custom Logger wrapper, which optimises out logging calls bellow the set verbosity level at compile time.
 *
 */
template <typename BaseLogger, logging::LoggerVerbosity verbosity>
class LoggerWrapper {
public:
	// Forward addSink to the base logger
	template <class T, typename... Args>
	static void addSink(Args &&...args) {
		BaseLogger::template addSink<T>(std::forward<Args>(args)...);
	}

	static void init(const logging::LoggerSettings &settings) {
		if (settings.verbosity != verbosity) {
			throw std::runtime_error("LoggerWrapper verbosity is different than the provided settings verbosity.");
		}
		BaseLogger::init(settings);
	}

	static void destroy() {
		BaseLogger::destroy();
	}

	// wrappers for common log levels
	template <typename... LogArgs>
	static void logDebug(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Debug) {
			return; // Skip logging if verbosity is lower than Debug
		}
		BaseLogger::log(logging::LoggerVerbosity::Debug, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void logInfo(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Info) {
			return; // Skip logging if verbosity is lower than Info
		}
		BaseLogger::log(logging::LoggerVerbosity::Info, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void logWarning(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Warning) {
			return; // Skip logging if verbosity is lower than Warning
		}
		BaseLogger::log(logging::LoggerVerbosity::Warning, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void logError(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Error) {
			return; // Skip logging if verbosity is lower than Error
		}
		BaseLogger::log(logging::LoggerVerbosity::Error, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void logCritical(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Critical) {
			return; // Skip logging if verbosity is lower than Critical
		}
		BaseLogger::log(logging::LoggerVerbosity::Critical, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void log(logging::LoggerVerbosity dynVerbosity, LogArgs &&...args) {
		if (verbosity > dynVerbosity) {
			return; // Skip logging if verbosity is lower than Critical
		}
		BaseLogger::log(dynVerbosity, std::forward<LogArgs>(args)...);
	}
};

}
