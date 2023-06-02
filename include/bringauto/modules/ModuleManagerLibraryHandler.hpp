#pragma once

#include <memory_management.h>
#include <device_management.h>

#include <functional>
#include <filesystem>



namespace bringauto::modules {

class ModuleManagerLibraryHandler {
public:
	ModuleManagerLibraryHandler() = default;
	~ModuleManagerLibraryHandler();

	int loadLibrary(const std::filesystem::path& path);

	std::function<int()> getModuleNumber {};
	std::function<int(unsigned int)> isDeviceTypeSupported {};
	std::function<int(struct buffer *, unsigned int)> generateFirstCommand {};
	std::function<int(struct buffer, unsigned int)> statusDataValid {};
	std::function<int(struct buffer, unsigned int)> commandDataValid {};
	std::function<int(struct buffer, struct buffer, unsigned int)> sendStatusCondition {};
	std::function<int(struct buffer *, struct buffer, struct buffer, unsigned int)> aggregateStatus {};
	std::function<int(struct buffer * error_message,
					const struct buffer current_error_message,
					const struct buffer status,
					unsigned int device_type)> aggregateError;
	std::function<int(struct buffer *, struct buffer, struct buffer, struct buffer, unsigned int)> generateCommand {};

private:
	void *module_ {};
};

}
