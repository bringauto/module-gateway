#pragma once

#include <string>
#include <vector>



namespace bringauto::common_utils {

/**
 * @brief Helper class for work with strings
 */
class StringUtils {
public:
	StringUtils() = delete;

	static std::vector<std::string> splitString(const std::string &input, char delimiter);

};

}