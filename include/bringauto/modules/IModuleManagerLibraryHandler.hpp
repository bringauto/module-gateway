#pragma once

#include <bringauto/modules/Buffer.hpp>

#include <filesystem>
#include <functional>



namespace bringauto::modules {

/**
 * @brief Class used to load and handle library created by module maintainer
 */
class IModuleManagerLibraryHandler {
public:
	IModuleManagerLibraryHandler() = default;

	virtual ~IModuleManagerLibraryHandler() = default;

	/**
	 * @brief Load library created by a module maintainer
	 *
	 * @param path path to the library
	 */
	virtual void loadLibrary(const std::filesystem::path &path) = 0;

	/**
	 * @brief Get the module number
	 *
	 * @return module number
	 */
	virtual int getModuleNumber() = 0;

	/**
	 * @brief Check if device type is supported by the module
	 *
	 * @param device_type device type to check
	 * @return OK if supported, NOT_OK otherwise
	 */
	virtual int isDeviceTypeSupported(unsigned int device_type) = 0;

	/**
	 * @brief Evaluate the condition for aggregating a new status into the current status
	 *
	 * @param current_status current aggregated status
	 * @param new_status newly received status
	 * @param device_type device type
	 * @return OK if the new status should be aggregated, NOT_OK otherwise
	 */
	virtual int sendStatusCondition(const Buffer &current_status, const Buffer &new_status, unsigned int device_type) = 0;

	/**
	 * @brief After executing the respective module function, an error might be thrown when allocating the buffer.
	 *
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	virtual int generateCommand(Buffer &generated_command, const Buffer &new_status,
								const Buffer &current_status, const Buffer &current_command,
								unsigned int device_type) = 0;

	/**
	 * @brief After executing the respective module function, an error might be thrown when allocating the buffer.
	 *
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	virtual int aggregateStatus(Buffer &aggregated_status, const Buffer &current_status,
								const Buffer &new_status, unsigned int device_type) = 0;

	/**
	 * @brief After executing the respective module function, an error might be thrown when allocating the buffer.
	 *
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	virtual int aggregateError(Buffer &error_message, const Buffer &current_error_message, const Buffer &status,
						  	   unsigned int device_type) = 0;

	/**
	 * @brief After executing the respective module function, an error might be thrown when allocating the buffer.
	 *
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	virtual int generateFirstCommand(Buffer &default_command, unsigned int device_type) = 0;

	/**
	 * @brief Check if the status data is valid
	 *
	 * @param status status buffer to validate
	 * @param device_type device type
	 * @return OK if valid, NOT_OK otherwise
	 */
	virtual int statusDataValid(const Buffer &status, unsigned int device_type) = 0;

	/**
	 * @brief Check if the command data is valid
	 *
	 * @param command command buffer to validate
	 * @param device_type device type
	 * @return OK if valid, NOT_OK otherwise
	 */
	virtual int commandDataValid(const Buffer &command, unsigned int device_type) = 0;

	/**
	 * @brief Constructs a buffer with the given size
	 *
	 * @param size size of the buffer
	 * @return a new Buffer object
	 */
	Buffer constructBuffer(std::size_t size = 0);

protected:

	std::function<void(struct buffer*)> deallocate_ {};

	virtual int allocate(struct buffer* buffer_pointer, size_t size_in_bytes) const = 0;

	/**
	 * @brief Constructs a buffer by taking ownership of the given raw C buffer
	 *
	 * @param buffer raw C buffer whose memory ownership is transferred
	 * @return a new Buffer object
	 */
	Buffer constructBufferByTakeOwnership(struct ::buffer& buffer);
};

}
