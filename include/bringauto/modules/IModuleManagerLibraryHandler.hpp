#pragma once

#include <bringauto/modules/Buffer.hpp>

#include <filesystem>



namespace bringauto::modules {

/**
 * @brief Class used to load and handle library created by module maintainer
 */
class IModuleManagerLibraryHandler {
public:
	explicit IModuleManagerLibraryHandler() = default;

	virtual ~IModuleManagerLibraryHandler() = default;

	/**
	 * @brief Load library created by a module maintainer
	 *
	 * @param path path to the library
	 */
	virtual void loadLibrary(const std::filesystem::path &path) = 0;

	virtual int getModuleNumber() = 0;

	virtual int isDeviceTypeSupported(unsigned int device_type) = 0;

	virtual int sendStatusCondition(const Buffer &current_status, const Buffer &new_status, unsigned int device_type) = 0;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	virtual int generateCommand(Buffer &generated_command, const Buffer &new_status,
								const Buffer &current_status, const Buffer &current_command,
								unsigned int device_type) = 0;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	virtual int aggregateStatus(Buffer &aggregated_status, const Buffer &current_status,
								const Buffer &new_status, unsigned int device_type) = 0;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	virtual int aggregateError(Buffer &error_message, const Buffer &current_error_message, const Buffer &status,
						  	   unsigned int device_type) = 0;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	virtual int generateFirstCommand(Buffer &default_command, unsigned int device_type) = 0;

	virtual int statusDataValid(const Buffer &status, unsigned int device_type) = 0;

	virtual int commandDataValid(const Buffer &command, unsigned int device_type) = 0;

	/**
	 * @brief Constructs a buffer with the given size
	 * 
	 * @param size size of the buffer
	 * @return a new Buffer object
	 */
	virtual Buffer constructBuffer(std::size_t size = 0) = 0;
};

}
