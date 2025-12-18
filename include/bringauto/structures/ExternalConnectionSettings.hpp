#pragma once

#include <string>
#include <unordered_map>
#include <vector>



namespace bringauto::structures {

/**
 * @brief Type of protocol used for transferring data
 */
enum class ProtocolType {
	INVALID = -1,
	MQTT,
	QUIC,
	DUMMY
};

struct ExternalConnectionSettings {
	/// Communication protocol
	ProtocolType protocolType {};
	/// Map of protocol specific settings, taken from config, a pair of key and value
	std::unordered_map<std::string, std::string> protocolSettings {};
	/// Ip address of the external server
	std::string serverIp {};
	/// Port of the external server
	u_int16_t port;
	/// Supported modules
	std::vector<int> modules {};
};

}
