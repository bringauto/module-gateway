#pragma once

#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>
#include <bringauto/structures/StatusAggregatorDeviceState.hpp>

#include <command_manager.h>
#include <module_manager.h>

#include <functional>
#include <map>
#include <queue>
#include <string>
#include <filesystem>
#include <mutex>



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

	/**
	 * @brief Allocates struct buffer with size
	 *
	 * @param buffer pointer to struct
	 * @param size_in_bytes
	 * @return 0 if success, otherwise -1
	 */
	int moduleAllocate(struct buffer *buffer, size_t size_in_bytes);

	/**
	 * @brief Deallocates struct buffer in loaded module
	 *
	 */
	void moduleDeallocate(struct buffer *buffer);

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

private:

	/**
	 * @brief Clear the device by string key
	 *
	 * @param device unique device key
	 * @return OK if success, otherwise NOT_OK
	 */
	int clear_device(const std::string &key);

	/**
	 * @brief Aggregate status message
	 *
	 * @param deviceState device state
	 * @param status status message
	 * @param device_type device type
	 * @return struct buffer with aggregated status message
	 */
	struct buffer aggregateStatus(structures::StatusAggregatorDeviceState &deviceState, const buffer &status,
								  const unsigned int &device_type);

	/**
	 * @brief Aggregate and set status message
	 *
	 * @param deviceState device state
	 * @param status status message
	 * @param device_type device type
	 */
	void aggregateSetStatus(structures::StatusAggregatorDeviceState &deviceState, const buffer &status,
							const unsigned int &device_type);

	/**
	 * @brief Aggregate, set and send status message
	 *
	 * @param deviceState device state
	 * @param status status message
	 * @param device_type device type
	 */
	void aggregateSetSendStatus(structures::StatusAggregatorDeviceState &deviceState, const buffer &status,
								const unsigned int &device_type);

	std::shared_ptr<structures::GlobalContext> context_;

	const std::shared_ptr<ModuleManagerLibraryHandler> module_ {};

	/**
	 * @brief Map of devices states, key is device identification coverted to string
	 */
	std::map<std::string, structures::StatusAggregatorDeviceState> devices {};

	std::mutex mutex_ {};

	std::atomic_bool timeoutedMessageReady_ { false };
};

}