#pragma once

#include <bringauto/modules/IModuleManagerLibraryHandler.hpp>



namespace bringauto::modules {

/**
 * @brief Class used to load and handle library created by module maintainer
 */
class ModuleManagerLibraryHandlerLocal : public IModuleManagerLibraryHandler {
public:
	ModuleManagerLibraryHandlerLocal() = default;

	~ModuleManagerLibraryHandlerLocal() override;

	/**
	 * @brief Loads the module shared library from the given path using dlmopen.
	 */
	void loadLibrary(const std::filesystem::path &path) override;

	int getModuleNumber() override;

	int isDeviceTypeSupported(unsigned int device_type) override;

	int sendStatusCondition(const Buffer &current_status, const Buffer &new_status, unsigned int device_type) override;

	int generateCommand(Buffer &generated_command, const Buffer &new_status,
						const Buffer &current_status, const Buffer &current_command,
						unsigned int device_type) override;

	int aggregateStatus(Buffer &aggregated_status, const Buffer &current_status,
						const Buffer &new_status, unsigned int device_type) override;

	int aggregateError(Buffer &error_message, const Buffer &current_error_message, const Buffer &status,
					   unsigned int device_type) override;

	int generateFirstCommand(Buffer &default_command, unsigned int device_type) override;

	int statusDataValid(const Buffer &status, unsigned int device_type) override;

	int commandDataValid(const Buffer &command, unsigned int device_type) override;

private:

	int allocate(struct buffer *buffer_pointer, size_t size_in_bytes) const override;

	void deallocate(struct buffer *buffer) const;

	void *checkFunction(const char *functionName) const;

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
};

}