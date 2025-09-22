#pragma once

#include <bringauto/modules/Buffer.hpp>

#include <fleet_protocol/common_headers/memory_management.h>

#include <functional>
#include <filesystem>



namespace bringauto::modules {

/**
 * @brief Class used to load and handle library created by module maintainer
 */
class ModuleManagerLibraryHandlerAsync {
public:
	ModuleManagerLibraryHandlerAsync();

	~ModuleManagerLibraryHandlerAsync();

	/**
	 * @brief Load library created by a module maintainer
	 *
	 * @param path path to the library
	 * @param moduleBinaryPath path to the module binary
	 */
	void loadLibrary(const std::filesystem::path &path, const std::string &moduleBinaryPath);

	int getModuleNumber() const;

	int isDeviceTypeSupported(unsigned int device_type) const;

	int	sendStatusCondition(const Buffer &current_status, const Buffer &new_status, unsigned int device_type) const;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int generateCommand(Buffer &generated_command, const Buffer &new_status,
						const Buffer &current_status, const Buffer &current_command,
						unsigned int device_type);

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int aggregateStatus(Buffer &aggregated_status, const Buffer &current_status,
						const Buffer &new_status, unsigned int device_type);

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int	aggregateError(Buffer &error_message, const Buffer &current_error_message, const Buffer &status,
						  unsigned int device_type);

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int generateFirstCommand(Buffer &default_command, unsigned int device_type);

	int statusDataValid(const Buffer &status, unsigned int device_type) const;

	int commandDataValid(const Buffer &command, unsigned int device_type) const;

	/**
	 * @brief Constructs a buffer with the given size
	 * 
	 * @param size size of the buffer
	 * @return a new Buffer object
	 */
	Buffer constructBuffer(std::size_t size = 0);

private:

	int allocate(struct buffer *buffer_pointer, size_t size_in_bytes) const;

	void deallocate(struct buffer *buffer) const;

	/**
	 * @brief Constructs a buffer with the same raw c buffer as provided
	 * 
	 * @param buffer c buffer to be used
	 * @return a new Buffer object
	 */
	Buffer constructBufferByTakeOwnership(struct ::buffer& buffer);

	std::function<void(struct buffer *)> deallocate_ {};

	/// Process id of the module binary
	pid_t moduleBinaryPid_ {};
};

}
