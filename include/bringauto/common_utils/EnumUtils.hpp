#pragma once

#include <bringauto/external_client/connection/ConnectionState.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <bringauto/logging/LoggerVerbosity.hpp>
#include <bringauto/settings/Constants.hpp>



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
	 * @param toString structures::ProtocolType
	 * @return std::string_view
	 */
	static constexpr std::string_view protocolTypeToString(structures::ProtocolType toString) {
		switch(toString) {
			case structures::ProtocolType::MQTT:
				return settings::Constants::MQTT;
			case structures::ProtocolType::QUIC:
				return settings::Constants::QUIC;
			case structures::ProtocolType::DUMMY:
				return settings::Constants::DUMMY;
			case structures::ProtocolType::INVALID:
			default:
				return "";
		}
	};

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
	 * @return std::string_view
	 */
	static constexpr std::string_view loggerVerbosityToString(logging::LoggerVerbosity verbosity) {
		switch(verbosity) {
			case logging::LoggerVerbosity::Debug:
				return settings::Constants::LOG_LEVEL_DEBUG;
			case logging::LoggerVerbosity::Info:
				return settings::Constants::LOG_LEVEL_INFO;
			case logging::LoggerVerbosity::Warning:
				return settings::Constants::LOG_LEVEL_WARNING;
			case logging::LoggerVerbosity::Error:
				return settings::Constants::LOG_LEVEL_ERROR;
			case logging::LoggerVerbosity::Critical:
				return settings::Constants::LOG_LEVEL_CRITICAL;
			default:
				return settings::Constants::LOG_LEVEL_INVALID;
		}
	};

	/**
	 * @brief Converts connection state to string
	 *
	 * @param toString structures::ProtocolType
	 * @return std::string_view
	 */
	static constexpr std::string_view connectionStateToString(external_client::connection::ConnectionState toString) {
		switch(toString) {
			case external_client::connection::ConnectionState::NOT_INITIALIZED: return "Not Initialized";
			case external_client::connection::ConnectionState::NOT_CONNECTED: return "Not Connected";
			case external_client::connection::ConnectionState::CONNECTING: return "Connecting";
			case external_client::connection::ConnectionState::CONNECTED: return "Connected";
			case external_client::connection::ConnectionState::CLOSING: return "Closing";
			default: return "Unknown";
		}
	};

};

}
