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

	/**
	 * @brief Split string by delimiter
	 *
	 * @param input string to split
	 * @param delimiter
	 * @return std::vector<std::string>
	 */
	static std::vector<std::string> splitString(const std::string &input, char delimiter);

};

}
