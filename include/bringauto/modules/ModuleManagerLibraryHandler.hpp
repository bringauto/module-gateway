#pragma once

#include <fleet_protocol/common_headers/memory_management.h>
#include <fleet_protocol/common_headers/device_management.h>

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
	sendStatusCondition(const struct buffer current_status, const struct buffer new_status, unsigned int device_type);

	int generateCommand(struct buffer *generated_command, const struct buffer new_status,
						const struct buffer current_status, const struct buffer current_command,
						unsigned int device_type);

	int aggregateStatus(struct buffer *aggregated_status, const struct buffer current_status,
						const struct buffer new_status, unsigned int device_type);

	int
	aggregateError(struct buffer *error_message, const struct buffer current_error_message, const struct buffer status,
				   unsigned int device_type);

	int generateFirstCommand(struct buffer *default_command, unsigned int device_type);

	int statusDataValid(const struct buffer status, unsigned int device_type);

	int commandDataValid(const struct buffer command, unsigned int device_type);

	int allocate(struct buffer *buffer_pointer, size_t size_in_bytes);

	void deallocate(struct buffer *buffer);

private:

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
