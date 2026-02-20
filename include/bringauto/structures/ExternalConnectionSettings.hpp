#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>



namespace bringauto::structures {

/**
 * @brief Type of protocol used for transferring data
 */
enum class ProtocolType {
	/// Not a valid protocol; used to signal parse errors
	INVALID = -1,
	/// MQTT protocol via Paho client
	MQTT,
	/// Dummy protocol for testing; simulates a connection without real I/O.
	/// NOTE: receiveMessage() always returns nullptr, so any ExternalConnection
	/// using DUMMY will never complete a connect sequence and will loop reconnecting.
	/// Use only in unit tests or send-only scenarios.
	DUMMY
};

struct ExternalConnectionSettings {
	/// Communication protocol
	ProtocolType protocolType { ProtocolType::INVALID };
	/// Map of protocol specific settings, taken from config, a pair of key and value
	std::unordered_map<std::string, std::string> protocolSettings {};
	/// Ip address of the external server
	std::string serverIp {};
	/// Port of the external server
	std::uint16_t port {};
	/// Supported modules
	std::vector<int> modules {};
};

}
