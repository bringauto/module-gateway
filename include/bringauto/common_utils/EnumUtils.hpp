#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>

namespace bringauto::common_utils {
/**
 * @brief Helper class for work with enums
 */
class EnumUtils {
public:
	EnumUtils() = delete;

	static structures::ProtocolType stringToProtocolType(std::string toEnum);

};

}