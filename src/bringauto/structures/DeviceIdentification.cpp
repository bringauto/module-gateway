#include <bringauto/structures/DeviceIdentification.hpp>
#include "bringauto/utils/utils.hpp"



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
	deviceRole_ = std::string(device.device_role);
	deviceName_ = std::string(device.device_name);
	priority_ = device.priority;
}

DeviceIdentification::DeviceIdentification(const std::string &deviceId) {
	std::vector <std::string> tokens = utils::splitString(deviceId, '/');
	module_ = std::stoi(tokens[0]);
	deviceType_ = static_cast<unsigned int>(std::stoi(tokens[1]));
	deviceRole_ = tokens[2];
	deviceName_ = tokens[3];
	priority_ = 0;
			// .priority = static_cast<unsigned int>(std::stoi(tokens[4]))
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

bool DeviceIdentification::isSame(const std::shared_ptr<DeviceIdentification> &toCompare) {
	return module_ == toCompare->getModule() &&
		   deviceType_ == toCompare->getDeviceType() &&
		   deviceRole_ == toCompare->getDeviceRole() &&
		   deviceName_ == toCompare->getDeviceName();
}

device_identification DeviceIdentification::convertToCStruct() const {
	return device_identification {
		.module = static_cast<int>(module_),
		.device_type = deviceType_,
		.device_role = const_cast<char *>(deviceRole_.c_str()),
		.device_name = const_cast<char *>(deviceName_.c_str()),
		.priority = priority_
	};
}


}