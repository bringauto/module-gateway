#include <bringauto/common_utils/StringUtils.hpp>

#include <sstream>



namespace bringauto::common_utils {

std::vector<std::string> StringUtils::splitString(const std::string &input, char delimiter) {
	std::vector<std::string> tokens;
	std::istringstream iss(input);
	std::string token;

	while(std::getline(iss, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

}