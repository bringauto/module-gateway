#pragma once

#include <format>
#include <iostream>
#include <map>
#include <vector>


namespace testing_utils {

class ConfigMock {
public:
	struct Config {
		struct Logging {
			struct Console {
				std::string level { "DEBUG" };
				bool use  { true };
			} console;
			struct File {
				std::string level { "DEBUG" };
				bool use { true };
				std::string path { "./" };
			} file;
		} logging;

		struct InternalServerSettings {
			int port { 1636 };
		} internal_server_settings;

		std::map<int, std::string> module_paths { {1, "/path/to/lib1.so"}, {2, "/path/to/lib2.so"}, {3, "/path/to/lib3.so"} };
		std::string modulePathsToString() const {
			std::string result = "";
			for (auto [key, value] : module_paths) {
				result += std::format("\"{}\": \"{}\",\n", key, value);
			}
			if (!result.empty()) {
				result.pop_back();
				result.pop_back();
			}
			return result;
		}

		struct ExternalConnection {
			std::string company { "bringauto" };
			std::string vehicle_name { "virtual_vehicle" };
			struct ExternalConnectionEndpoint {
				std::string protocol_type { "mqtt" };
				std::string server_ip { "localhost" };
				int port { 1883 };
				
				std::map<std::string, std::string> mqtt_settings {
					{ "ssl", "\"false\"" },
					{ "ca-file", "\"ca.pem\"" },
					{ "client-cert", "\"client.pem\"" },
					{ "client", "\"key.pem\"" }
				};
				std::string mqttSettingsToString() const {
					std::string result = "";
					for (auto [key, value] : mqtt_settings) {
						result += std::format("\"{}\": {},\n", key, value);
					}
					if (!result.empty()) {
						result.pop_back();
						result.pop_back();
					}
					return result;
				}
				
				std::vector<int> modules { 1, 2, 3 };
				std::string modulesToString() const {
					std::string result = "";
					for (auto module : modules) {
						result += std::format("{},", module);
					}
					if (!result.empty()) {
						result.pop_back();
					}
					return result;
				}
			} endpoint;
		} external_connection;
	};

	

	ConfigMock(const Config &config) : config_(config) {
		auto endpoint = config_.external_connection.endpoint;

		configString_ = std::format(
			"{{\n"
				"\"logging\": {{\n"
					"\"console\": {{\n"
						"\"level\": \"{}\",\n"
						"\"use\": {}\n"
					"}},\n"
					"\"file\": {{\n"
						"\"level\": \"{}\",\n"
						"\"use\": {},\n"
						"\"path\": \"{}\"\n"
					"}}\n"
				"}},\n"
				"\"internal-server-settings\": {{\n"
					"\"port\": {}\n"
				"}},\n"
				"\"module-paths\": {{\n"
					"{}\n"
				"}},\n"
				"\"external-connection\": {{\n"
					"\"company\": \"{}\",\n"
					"\"vehicle-name\": \"{}\",\n"
					"\"endpoints\":\n"
					"[\n"
						"{{\n"
							"\"protocol-type\": \"{}\",\n"
							"\"server-ip\": \"{}\",\n"
							"\"port\": {},\n"
							"\"mqtt-settings\": {{\n"
								"{}"
							"}},\n"
							"\"modules\": [{}]\n"
						"}}\n"
					"]\n"
				"}}\n"
			"}}",
			config_.logging.console.level, boolToString(config_.logging.console.use),
			config_.logging.file.level, boolToString(config_.logging.file.use), config_.logging.file.path,
			config_.internal_server_settings.port,
			config_.modulePathsToString(),
			config_.external_connection.company, config_.external_connection.vehicle_name,
			endpoint.protocol_type, endpoint.server_ip, endpoint.port,
			endpoint.mqttSettingsToString(),
			endpoint.modulesToString()
		);
	}

	std::string getConfigString() const {
		return configString_;
	}

private:
	std::string boolToString(bool value) const {
		return value ? "true" : "false";
	}

	Config config_ {};
	std::string configString_;
};

}
