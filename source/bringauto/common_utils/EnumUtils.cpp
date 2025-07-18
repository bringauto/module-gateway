#include <bringauto/common_utils/EnumUtils.hpp>

#include <algorithm>
#include <iostream>


namespace bringauto::common_utils {

structures::ProtocolType EnumUtils::stringToProtocolType(std::string toEnum) {
	std::transform(toEnum.begin(), toEnum.end(), toEnum.begin(), ::toupper);
	if(toEnum == settings::Constants::MQTT) {
		return structures::ProtocolType::MQTT;
	} else if(toEnum == settings::Constants::DUMMY) {
        return structures::ProtocolType::DUMMY;
    }
	return structures::ProtocolType::INVALID;
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

}
