#include <bringauto/common_utils/EnumUtils.hpp>
#include <bringauto/settings/Constants.hpp>

#include <algorithm>
#include <iostream>


namespace bringauto::common_utils {

structures::ProtocolType EnumUtils::stringToProtocolType(std::string toEnum) {
	std::transform(toEnum.begin(), toEnum.end(), toEnum.begin(), ::toupper);
	if(toEnum == settings::Constants::MQTT) {
		return structures::ProtocolType::MQTT;
	}
	return structures::ProtocolType::INVALID;
}

std::string EnumUtils::protocolTypeToString(structures::ProtocolType toString) {
	std::string result {};
	if(toString == structures::ProtocolType::MQTT) {
		result = settings::Constants::MQTT;
	}
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

logging::LoggerVerbosity EnumUtils::stringToLoggerVerbosity(std::string toEnum) {
	std::transform(toEnum.begin(), toEnum.end(), toEnum.begin(), ::toupper);
	if(toEnum == settings::Constants::LOG_LEVEL_DEBUG) {
		return logging::LoggerVerbosity::Debug;
	} else if(toEnum == settings::Constants::LOG_LEVEL_INFO) {
		return logging::LoggerVerbosity::Info;
	} else if(toEnum == settings::Constants::LOG_LEVEL_WARNING) {
		return logging::LoggerVerbosity::Warning;
	} else if(toEnum == settings::Constants::LOG_LEVEL_ERROR) {
		return logging::LoggerVerbosity::Error;
	} else if(toEnum == settings::Constants::LOG_LEVEL_CRITICAL) {
		return logging::LoggerVerbosity::Critical;
	}
	std::cerr << "Invalid logger verbosity: " << toEnum << std::endl;
	return logging::LoggerVerbosity::Debug;
}

std::string EnumUtils::loggerVerbosityToString(logging::LoggerVerbosity verbosity) {
	std::string result {};
	switch(verbosity) {
		case logging::LoggerVerbosity::Debug:
			result = settings::Constants::LOG_LEVEL_DEBUG;
			break;
		case logging::LoggerVerbosity::Info:
			result = settings::Constants::LOG_LEVEL_INFO;
			break;
		case logging::LoggerVerbosity::Warning:
			result = settings::Constants::LOG_LEVEL_WARNING;
			break;
		case logging::LoggerVerbosity::Error:
			result = settings::Constants::LOG_LEVEL_ERROR;
			break;
		case logging::LoggerVerbosity::Critical:
			result = settings::Constants::LOG_LEVEL_CRITICAL;
			break;
		default:
			result = settings::Constants::LOG_LEVEL_INVALID;
			break;
	}
	return result;
}
}
