#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <bringauto/logging/LoggerVerbosity.hpp>



namespace bringauto::common_utils {
/**
 * @brief Helper class for work with enums
 */
class EnumUtils {
public:
	EnumUtils() = delete;

	/**
	 * @brief Converts string to protocol type
	 *
	 * @param toEnum string
	 * @return structures::ProtocolType
	 */
	static structures::ProtocolType stringToProtocolType(std::string toEnum);

	/**
	 * @brief Converts protocol type to string
	 *
	 * @param toEnum structures::ProtocolType
	 * @return std::string
	 */
	static std::string protocolTypeToString(structures::ProtocolType toString);

	/**
	 * @brief Converts string to logger verbosity
	 * 
	 * @param toEnum string
	 * @return logging::LoggerVerbosity
	 */
	static logging::LoggerVerbosity stringToLoggerVerbosity(std::string toEnum);

	/**
	 * @brief Converts logger verbosity to string
	 *
	 * @param verbosity logging::LoggerVerbosity
	 * @return std::string
	 */
	static std::string loggerVerbosityToString(logging::LoggerVerbosity verbosity);

};

}
