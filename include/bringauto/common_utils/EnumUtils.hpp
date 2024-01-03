#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>



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

};

}
