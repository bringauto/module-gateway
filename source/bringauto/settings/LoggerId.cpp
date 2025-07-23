#include <bringauto/settings/LoggerId.hpp>

namespace {

/**
 * @brief Compile time string comparison
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @return true if both strings are equal, false otherwise
 */
constexpr bool stringsEqual(char const *str1, char const *str2) {
	return std::string_view(str1) == str2;
}

static_assert(stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "DEBUG") ||
			  stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "INFO") ||
			  stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "WARNING") ||
			  stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "ERROR") ||
			  stringsEqual(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY, "CRITICAL"),
			  "Invalid MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY defined. Use DEBUG, INFO, WARNING, ERROR, or CRITICAL.");

}