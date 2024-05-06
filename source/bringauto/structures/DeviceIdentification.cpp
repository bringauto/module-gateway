#include <bringauto/structures/DeviceIdentification.hpp>
#include <bringauto/common_utils/StringUtils.hpp>
#include <bringauto/common_utils/MemoryUtils.hpp>



namespace bringauto::structures {
DeviceIdentification::DeviceIdentification(const InternalProtocol::Device &device) {
	module_ = device.module();
	deviceType_ = device.devicetype();
	deviceRole_ = device.devicerole();
	deviceName_ = device.devicename();
	priority_ = device.priority();
}

DeviceIdentification::DeviceIdentification(const device_identification &device) {
	module_ = device.module;
	deviceType_ = device.device_type;
	deviceRole_ = std::string { static_cast<char *>(device.device_role.data), device.device_role.size_in_bytes };
	deviceName_ = std::string { static_cast<char *>(device.device_name.data), device.device_name.size_in_bytes };
	priority_ = device.priority;
}

uint32_t DeviceIdentification::getPriority() const {
	return priority_;
}

uint32_t DeviceIdentification::getModule() const {
	return module_;
}

uint32_t DeviceIdentification::getDeviceType() const {
	return deviceType_;
}

const std::string &DeviceIdentification::getDeviceRole() const {
	return deviceRole_;
}

const std::string &DeviceIdentification::getDeviceName() const {
	return deviceName_;
}

bool DeviceIdentification::isSame(const std::shared_ptr<DeviceIdentification> &toCompare) const {
	return module_ == toCompare->getModule() &&
		   deviceType_ == toCompare->getDeviceType() &&
		   deviceRole_ == toCompare->getDeviceRole();
}

DeviceIdentification& DeviceIdentification::operator=(const InternalProtocol::Device &device) {
	module_ = device.module();
	deviceType_ = device.devicetype();
	deviceRole_ = device.devicerole();
	deviceName_ = device.devicename();
	priority_ = device.priority();
	return *this;
}

bool DeviceIdentification::operator==(const DeviceIdentification &deviceId) const {
	return module_ == deviceId.getModule() &&
		   deviceType_ == deviceId.getDeviceType() &&
		   deviceRole_ == deviceId.getDeviceRole();
}

InternalProtocol::Device DeviceIdentification::convertToIPDevice() const {
	InternalProtocol::Device device;
	device.set_module(static_cast<InternalProtocol::Device::Module>(module_));
	device.set_devicetype(deviceType_);
	device.set_devicerole(deviceRole_);
	device.set_devicename(deviceName_);
	device.set_priority(priority_);
	return device;
}


}