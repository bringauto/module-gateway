#pragma once

#include <libbringauto_logger/bringauto/logging/Logger.hpp>

#include <string>
#include <string_view>



namespace bringauto::settings {

/**
 * @brief Custom Logger wrapper, which optimises out logging calls below the set verbosity level at compile time.
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

	/**
	 * @brief Initialize the logger with the given name.
	 * The verbosity is always set to the compile-time template parameter — it is not
	 * configurable at runtime. Sink-level verbosity filters are set separately via addSink().
	 * @param loggerName name of the logger instance
	 */
	static void init(std::string_view loggerName) {
		BaseLogger::init(logging::LoggerSettings{ std::string(loggerName), verbosity });
	}

	static void destroy() {
		BaseLogger::destroy();
	}

	// wrappers for common log levels
	template <typename... LogArgs>
	static void logDebug(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Debug) {
			return; // compile-time minimum exceeds Debug; call eliminated by optimizer
		}
		BaseLogger::log(logging::LoggerVerbosity::Debug, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void logInfo(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Info) {
			return; // compile-time minimum exceeds Info; call eliminated by optimizer
		}
		BaseLogger::log(logging::LoggerVerbosity::Info, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void logWarning(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Warning) {
			return; // compile-time minimum exceeds Warning; call eliminated by optimizer
		}
		BaseLogger::log(logging::LoggerVerbosity::Warning, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void logError(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Error) {
			return; // compile-time minimum exceeds Error; call eliminated by optimizer
		}
		BaseLogger::log(logging::LoggerVerbosity::Error, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void logCritical(LogArgs &&...args) {
		if constexpr (verbosity > logging::LoggerVerbosity::Critical) {
			return; // compile-time minimum exceeds Critical; call eliminated by optimizer
		}
		BaseLogger::log(logging::LoggerVerbosity::Critical, std::forward<LogArgs>(args)...);
	}

	template <typename... LogArgs>
	static void log(logging::LoggerVerbosity dynVerbosity, LogArgs &&...args) {
		if (verbosity > dynVerbosity) {
			return; // runtime check: message level is below compile-time minimum
		}
		BaseLogger::log(dynVerbosity, std::forward<LogArgs>(args)...);
	}
};

}
