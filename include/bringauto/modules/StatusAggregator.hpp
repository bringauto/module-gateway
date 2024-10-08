#pragma once

#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>
#include <bringauto/structures/StatusAggregatorDeviceState.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>

#include <fleet_protocol/module_gateway/command_manager.h>
#include <fleet_protocol/module_maintainer/module_gateway/module_manager.h>

#include <functional>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <list>



namespace bringauto::modules {

/**
 * @brief Class created from file status_aggregator.h in fleet-protocol
 *
 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
 */
class StatusAggregator {
public:

	explicit StatusAggregator(const std::shared_ptr<structures::GlobalContext> &context,
							  const std::shared_ptr<ModuleManagerLibraryHandler> &libraryHandler): context_ { context },
																								   module_ {
																										   libraryHandler } {};

	StatusAggregator() = default;

	/**
	 * @short Status aggregator init.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int init_status_aggregator();

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
	int clear_device(const structures::DeviceIdentification& device);

	/**
	 * @short Remove device from aggregator
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int remove_device(const structures::DeviceIdentification& device);

	/**
	 * @short Add protobuf status message to aggregator, aggregator will aggregate each unique devices messages of
	 * type device_type separately
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int add_status_to_aggregator(const Buffer& status, const structures::DeviceIdentification& device);

	/**
	 * @short Get the oldest aggregated protobuf status message that is aggregated
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int get_aggregated_status(Buffer &generated_status, const structures::DeviceIdentification& device);

	/**
	 * @short Get all devices registered to aggregator. This specific implementation takes a list
	 * of DeviceIdentification structures and fills it with all registered devices (as opposed to
	 * the function declaration, which uses a buffer structure). This is equivalent to the
	 * intended use of the function.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int get_unique_devices(std::list<structures::DeviceIdentification> &unique_devices_list);

	/**
	 * @short Force status message aggregation on given device.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int force_aggregation_on_device(const structures::DeviceIdentification& device);

	/**
	 * @short Check if device is valid and registered
	 *
	 * @see fleet-protocol/lib/module_gateway/include/status_aggregator.h
	 */
	int is_device_valid(const structures::DeviceIdentification& device);

	/**
	 * @short Update command message.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/command_manager.h
	 */
	int update_command(const Buffer& command, const structures::DeviceIdentification& device);

	/**
	 * @short Get command message.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/command_manager.h
	 */
	int get_command(const Buffer& status, const structures::DeviceIdentification& device,
					Buffer &command);

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

	/**
	 * @brief Unset timeouted message ready
	 *
	 */
	void unsetTimeoutedMessageReady();

	/**
	 * @brief Get the timeouted message ready
	 *
	 * @return true if force timeout was called on any device otherwise false
	 */
	bool getTimeoutedMessageReady() const;

	/**
	 * @brief Get the device timeout count
	 *
	 * @param key device unique key, obtained from getId function in ProtobufUtils
	 * @return number of timeouts
	 */
	int getDeviceTimeoutCount(const structures::DeviceIdentification& );

private:

	/**
	 * @brief Aggregate status message
	 *
	 * @param deviceState device state
	 * @param status status message
	 * @param device_type device type
	 * @return struct buffer with aggregated status message
	 */
	Buffer aggregateStatus(structures::StatusAggregatorDeviceState &deviceState,
											   const Buffer &status,
											   const unsigned int &device_type);

	/**
	 * @brief Aggregate and set status message
	 *
	 * @param deviceState device state
	 * @param status status message
	 * @param device_type device type
	 */
	void aggregateSetStatus(structures::StatusAggregatorDeviceState &deviceState, const Buffer &status,
							const unsigned int &device_type);

	/**
	 * @brief Aggregate, set and send status message
	 *
	 * @param deviceState device state
	 * @param status status message
	 * @param device_type device type
	 */
	void aggregateSetSendStatus(structures::StatusAggregatorDeviceState &deviceState, const Buffer &status,
								const unsigned int &device_type);

	std::shared_ptr<structures::GlobalContext> context_ {};

	const std::shared_ptr<ModuleManagerLibraryHandler> module_ {};

	/**
	 * @brief Map of devices states, key is device identification
	 */
	std::unordered_map<structures::DeviceIdentification, structures::StatusAggregatorDeviceState> devices {};

	/**
	 * @brief Map of devices timeouts, key is device identification converted to string
	 */
	std::unordered_map<structures::DeviceIdentification, int> deviceTimeouts_ {};

	std::atomic_bool timeoutedMessageReady_ { false };
};

}
