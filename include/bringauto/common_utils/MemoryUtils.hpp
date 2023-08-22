#pragma once

#include <memory_management.h>
#include <device_management.h>

#include <string>



namespace bringauto::common_utils {

class MemoryUtils {
public:
	MemoryUtils() = delete;

	static void initBuffer(struct buffer &buffer, const std::string &data);

	static void deallocateDeviceId(struct ::device_identification &device);
};

}