#pragma once

#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <memory_management.h>
#include <device_management.h>

#include <functional>
#include <filesystem>
#include <map>



namespace bringauto::external_client {

/**
 * @brief Class created from file error_aggregator.h in fleet-protocol
 *
 * @see fleet-protocol/lib/module_gateway/include/error_aggregator.h
 */
class ErrorAggregator {
public:
	/**
	 * @short Error aggregator init.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/error_aggregator.h
	 *
	 * @return OK if initialization was successful
	 * @return NOT_OK if an error occurred
	 */
	int init_error_aggregator(const std::shared_ptr <modules::ModuleManagerLibraryHandler> &library);

	/**
	 * @short Clean up.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/error_aggregator.h
	 *
	 * @return OK if successful
	 * @return NOT_OK if an error occurred
	 */
	int destroy_error_aggregator();

	/**
	 * @short Add protobuf status message to an error aggregator, aggregator will aggregate each unique devices messages of type device_type separately.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/error_aggregator.h
	 *
	 * @param status protobuf status message in binary form
	 * @param device identification of the device
	 *
	 * @return OK if successful
	 * @return DEVICE_NOT_REGISTERED if device is not registered
	 * @return NOT_OK for other error
	 */
	int add_status_to_error_aggregator(const struct buffer status, const struct device_identification device);

	/**
	 * @short Get status from error aggregator for a specific device.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/error_aggregator.h
	 *
	 * @param status user initialized message_buffer for the error status. Look at 'memory management' section
	 * @param device identification of the device
	 *
	 * @return OK if successful
	 * @return NO_MESSAGE_AVAILABLE if no message was generated
	 * @return DEVICE_NOT_REGISTERED if device was not registered
	 * @return NOT_OK for other errors
	 */
	int get_last_status(struct buffer *status, const struct device_identification device);

	/**
	 * @short Get error message from error aggregator for a specific device.
	 *
	 * @see fleet-protocol/lib/module_gateway/include/error_aggregator.h
	 *
	 * @param error user initialized message_buffer for created protobuf error status. Look at 'memory management' section.
	 * @param device identification of the device
	 *
	 * @return OK if successful
	 * @return NO_MESSAGE_AVAILABLE if no message was generated
	 * @return DEVICE_NOT_REGISTERED if device was no registered
	 * @return NOT_OK for other errors
	 */
	int get_error(struct buffer *error, const struct device_identification device);

	/**
	 * @short Clear error aggregator
	 *
	 * All messages for all registered devices will be removed.
	 *
	 * @return OK if successful
	 * @return NOT_OK if an error occurs
	 */
	int clear_error_aggregator();

	/**
	 * @short Get number of the module
	 *
	 * @see fleet-protocol/lib/common_headers/include/device_management.h
	 */
	int get_module_number() const;

	/**
     * @short Check if device is supported
     *
     * @see fleet-protocol/lib/common_headers/include/device_management.h
     */
	int is_device_type_supported(unsigned int device_type);

private:
	struct DeviceState {
		struct buffer errorMessage {};
		struct buffer lastStatus {};

		DeviceState() {
			errorMessage.data = nullptr;
			errorMessage.size_in_bytes = 0;
			lastStatus.data = nullptr;
			lastStatus.size_in_bytes = 0;
		};
	};

	std::shared_ptr <modules::ModuleManagerLibraryHandler> module_ {};

	std::map <std::string, DeviceState> devices_ {};
};

}
