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
	 * @param state external_client::connection::ConnectionState
	 * @return std::string_view
	 */
	static constexpr std::string_view connectionStateToString(
		external_client::connection::ConnectionState state
	) {
		using enum external_client::connection::ConnectionState;

		switch (state) {
			case NOT_INITIALIZED:
				return settings::Constants::LOG_CONNECTION_STATE_NOT_INITIALIZED;
			case NOT_CONNECTED:
				return settings::Constants::LOG_CONNECTION_STATE_NOT_CONNECTED;
			case CONNECTING:
				return settings::Constants::LOG_CONNECTION_STATE_CONNECTING;
			case CONNECTED:
				return settings::Constants::LOG_CONNECTION_STATE_CONNECTED;
			case CLOSING:
				return settings::Constants::LOG_CONNECTION_STATE_CLOSING;
			default:
				return settings::Constants::LOG_UNKNOWN;
		}
	}
	};
}
