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
	sendStatusCondition(const bringauto::modules::Buffer current_status, const bringauto::modules::Buffer new_status, unsigned int device_type);

	int generateCommand(bringauto::modules::Buffer &generated_command, const bringauto::modules::Buffer new_status,
						const bringauto::modules::Buffer current_status, const bringauto::modules::Buffer current_command,
						unsigned int device_type);

	int aggregateStatus(bringauto::modules::Buffer &aggregated_status, const bringauto::modules::Buffer current_status,
						const bringauto::modules::Buffer new_status, unsigned int device_type);

	int
	aggregateError(bringauto::modules::Buffer &error_message, const bringauto::modules::Buffer current_error_message, const bringauto::modules::Buffer status,
				   unsigned int device_type);

	int generateFirstCommand(bringauto::modules::Buffer &default_command, unsigned int device_type);

	int statusDataValid(const bringauto::modules::Buffer status, unsigned int device_type);

	int commandDataValid(const bringauto::modules::Buffer command, unsigned int device_type);

	bringauto::modules::Buffer constructBufferByAllocate(std::size_t size) {
		struct ::buffer buff;
		buff.size_in_bytes = size;
		if(allocate(&buff, size) != OK) {
			throw std::runtime_error("Could not allocate memory for buffer");
		}
		return { buff, deallocate_ };
	}

	bringauto::modules::Buffer constructBufferByTakeOwnership(struct ::buffer& buffer) {
		return { buffer, deallocate_ };
	}

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
