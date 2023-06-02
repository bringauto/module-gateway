#pragma once
#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <functional>
#include <map>
#include <queue>
#include <string>
#include <filesystem>

#include <command_manager.h>
#include <module_manager.h>



namespace bringauto::modules {

/**
 * @brief Class created from file status_aggregator.h in fleet-protocol
 *
 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
 */
class StatusAggregator {
public:
	/**
	 * @short Status aggregator init.
	 *
     * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int init_status_aggregator(const ModuleManagerLibraryHandler &library);

	/**
	 * @short Clean up.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int destroy_status_aggregator(); /// module specific

	/**
	 * @short Clear states and messages for all devices.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int clear_all_devices();

	/**
	 * @short Clear state and messages for given device
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int clear_device(const struct ::device_identification device);

	/**
	 * @short Remove device from aggregator
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int remove_device(const struct ::device_identification device);

	/**
	 * @short Add protobuf status message to aggregator, aggregator will aggregate each unique devices messages of
	 * type device_type separately
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int add_status_to_aggregator(const struct ::buffer status, const struct ::device_identification device);

	/**
	 * @short Get the oldest aggregated protobuf status message that is aggregated
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int get_aggregated_status(struct ::buffer *generated_status, const struct ::device_identification device);

	/**
	 * @short Get all devices registered to aggregator
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int get_unique_devices(struct ::buffer *unique_devices_buffer);

	/**
	 * @short Force status message aggregation on given device.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int force_aggregation_on_device(const struct ::device_identification device);

	/**
     * @short Check if device is valid and registered
     *
     * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
     */
	int is_device_valid(const struct ::device_identification device);

	/**
	 * @short Update command message.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/command_manager.h
	 */
	int update_command(const struct ::buffer command, const struct ::device_identification device);

	/**
	 * @short Get command message.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/command_manager.h
	 */
	int get_command(const struct ::buffer status, const struct ::device_identification device,
					struct ::buffer *command);

	/**
	 * @short Get number of the module
	 *
	 * @see fleet-protocol/lib/common_headers/include/device_management.h
	 */
	int get_module_number();

	/**
     * @short Check if device is supported
     *
     * @see fleet-protocol/lib/common_headers/include/device_management.h
     */
	int is_device_type_supported(unsigned int device_type);

private:
	struct device_state {
		std::queue<struct buffer> aggregatedMessages;
		struct buffer status;
		struct buffer command;

		device_state() = default;

		device_state(const buffer command, const buffer status) {
			this->status = status;
			this->command = command;
		}
	};

	std::string getId(const ::device_identification &device);

	ModuleManagerLibraryHandler module { };

	std::map <std::string, device_state> devices {};
};

}