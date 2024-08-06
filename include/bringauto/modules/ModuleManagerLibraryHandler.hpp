#pragma once

#include <bringauto/modules/Buffer.hpp>

#include <fleet_protocol/common_headers/memory_management.h>
#include <fleet_protocol/common_headers/device_management.h>
#include <fleet_protocol/common_headers/general_error_codes.h>

#include <functional>
#include <filesystem>



namespace bringauto::modules {

/**
 * @brief Class used to load and handle library created by module maintainer
 */
class ModuleManagerLibraryHandler {
public:
	ModuleManagerLibraryHandler() = default;

	~ModuleManagerLibraryHandler();

	/**
	 * @brief Load library created by a module maintainer
	 *
	 * @param path path to the library
	 */
	void loadLibrary(const std::filesystem::path &path);

	int getModuleNumber();

	int isDeviceTypeSupported(unsigned int device_type);

	int
	sendStatusCondition(const Buffer current_status, const Buffer new_status, unsigned int device_type);

	int generateCommand(Buffer &generated_command, const Buffer new_status,
						const Buffer current_status, const Buffer current_command,
						unsigned int device_type);

	int aggregateStatus(Buffer &aggregated_status, const Buffer current_status,
						const Buffer new_status, unsigned int device_type);

	int
	aggregateError(Buffer &error_message, const Buffer current_error_message, const Buffer status,
				   unsigned int device_type);

	int generateFirstCommand(Buffer &default_command, unsigned int device_type);

	int statusDataValid(const Buffer status, unsigned int device_type);

	int commandDataValid(const Buffer command, unsigned int device_type);

	/**
	 * @brief Constructs a buffer with the given size
	 * 
	 * @param size size of the buffer
	 * @return a new Buffer object
	 */
	Buffer constructBuffer(std::size_t size = 0);

	/**
	 * @brief Constructs a buffer with the same raw c buffer as provided
	 * 
	 * @param buffer c buffer to be used
	 * @return a new Buffer object
	 */
	Buffer constructBufferByTakeOwnership(struct ::buffer& buffer);

private:

	int allocate(struct buffer *buffer_pointer, size_t size_in_bytes);

	void deallocate(struct buffer *buffer);

	void *checkFunction(const char *functionName);

	void *module_ {};

	std::function<int()> getModuleNumber_ {};
	std::function<int(unsigned int)> isDeviceTypeSupported_ {};
	std::function<int(struct buffer *, unsigned int)> generateFirstCommand_ {};
	std::function<int(const struct buffer, unsigned int)> statusDataValid_ {};
	std::function<int(const struct buffer, unsigned int)> commandDataValid_ {};
	std::function<int(struct buffer, struct buffer, unsigned int)> sendStatusCondition_ {};
	std::function<int(struct buffer *, struct buffer, struct buffer, unsigned int)> aggregateStatus_ {};
	std::function<int(struct buffer *error_message,
					  const struct buffer current_error_message,
					  const struct buffer status,
					  unsigned int device_type)> aggregateError_ {};
	std::function<int(struct buffer *, struct buffer, struct buffer, struct buffer, unsigned int)> generateCommand_ {};
	std::function<int(struct buffer *, size_t)> allocate_ {};
	std::function<void(struct buffer *)> deallocate_ {};
};

}
