#pragma once

#include <string>
#include <map>
#include <vector>

namespace bringauto::structures {

enum class ProtocolType {
	INVALID = -1,
	MQTT
};

struct ExternalConnectionSettings {
	ProtocolType protocolType;
	std::map<std::string, std::string> protocolSettings;
	std::string serverIp;
	u_int16_t port;
	std::vector<int> modules;

};
}