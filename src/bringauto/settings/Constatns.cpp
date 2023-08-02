#include <bringauto/settings/Constants.hpp>

namespace bringauto::settings {

    const std::string Constants::CONFIG_PATH { "config-path" };
    const std::string Constants::VERBOSE { "verbose" };
	const std::string Constants::LOG_PATH { "log-path" };
	const std::string Constants::HELP { "help" };
	const std::string Constants::PORT { "port" };

	const std::string Constants::MODULE_PATHS { "module-paths" };

	const std::string Constants::GENERAL_SETTINGS { "general-settings" };
	const std::string Constants::INTERNAL_SERVER_SETTINGS { "internal-server-settings" };

	const std::string Constants::EXTERNAL_CONNECTION { "external-connection" };
	const std::string Constants::VEHICLE_NAME { "vehicle-name" };
	const std::string Constants::COMPANY { "company" };
	const std::string Constants::EXTERNAL_ENDPOINTS { "endpoints" };
	const std::string Constants::SERVER_IP { "server-ip" };
	const std::string Constants::PROTOCOL_TYPE { "protocol-type" };

	const std::string Constants::MQTT_SETTINGS { "mqtt-settings" };
	const std::string Constants::SSL { "ssl" };
	const std::string Constants::CA_FILE { "ca-file" };
	const std::string Constants::CLIENT_CERT { "client-cert" };
	const std::string Constants::CLIENT_KEY { "client-key" };

	const std::string Constants::MODULES { "modules" };
}