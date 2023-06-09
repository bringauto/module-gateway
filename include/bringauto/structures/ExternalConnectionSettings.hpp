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
	/**
	 * Communication protocol
	 */
    ProtocolType protocolType;

	std::map<std::string, std::string> protocolSettings;

    std::string serverIp;

    u_int16_t port;

    /**
     * Supported modules
     */
	std::vector<int> modules;
};

}