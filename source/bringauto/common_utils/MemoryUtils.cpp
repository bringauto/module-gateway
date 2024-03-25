#include <bringauto/common_utils/MemoryUtils.hpp>

#include <cstring>



namespace bringauto::common_utils {

void MemoryUtils::initBuffer(struct buffer &buffer, const std::string &data) {
	allocate(&buffer, data.size());
	std::memcpy(buffer.data, data.c_str(), data.size());
}

void MemoryUtils::deallocateDeviceId(struct ::device_identification &device) {
	deallocate(&device.device_role);
	deallocate(&device.device_name);
}

}