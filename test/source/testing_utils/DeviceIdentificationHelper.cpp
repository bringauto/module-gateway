#include <testing_utils/DeviceIdentificationHelper.h>

namespace testing_utils {

namespace structures = bringauto::structures;


structures::DeviceIdentification
DeviceIdentificationHelper::createDeviceIdentification(unsigned int module, unsigned int type, const char *deviceRole,
                                                       const char *deviceName, int priority) {
    auto device = createProtobufDevice(module, type, deviceRole, deviceName, priority);
    return structures::DeviceIdentification(device);
}

InternalProtocol::Device
DeviceIdentificationHelper::createProtobufDevice(unsigned int module, unsigned int type, const char *deviceRole,
                                                 const char *deviceName, int priority) {
    InternalProtocol::Device device {};
    device.set_module(InternalProtocol::Device::Module(module));
    device.set_devicetype(type);
    device.set_priority(priority);
    device.set_devicerole(deviceRole);
    device.set_devicename(deviceName);
    return device;
}

}