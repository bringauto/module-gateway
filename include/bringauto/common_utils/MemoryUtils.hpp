#pragma once

#include <memory_management.h>
#include <device_management.h>

#include <string>



namespace bringauto::common_utils {

class MemoryUtils {
public:
	MemoryUtils() = delete;

	/**
	 * @brief Allocate, initialize and fill buffer with data
	 *
	 * @param buffer buffer to initialize
	 * @param data data to fill buffer with
	 */
	static void initBuffer(struct buffer &buffer, const std::string &data);

	/**
	 * @brief Deallocate struct device_identification
	 *
	 * @param device struct to deallocate
	 */
	static void deallocateDeviceId(struct ::device_identification &device);
};

}
