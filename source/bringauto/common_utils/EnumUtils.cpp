#include <bringauto/common_utils/EnumUtils.hpp>
#include <bringauto/settings/Constants.hpp>

#include <algorithm>



namespace bringauto::common_utils {

structures::ProtocolType EnumUtils::stringToProtocolType(std::string toEnum) {
	std::transform(toEnum.begin(), toEnum.end(), toEnum.begin(), ::toupper);
	if(toEnum == settings::Constants::MQTT) {
		return structures::ProtocolType::MQTT;
	}
	return structures::ProtocolType::INVALID;
}
}