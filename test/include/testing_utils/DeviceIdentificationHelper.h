#pragma once

#include <bringauto/structures/DeviceIdentification.hpp>

namespace testing_utils {
    class DeviceIdentificationHelper {
    public:
        static bringauto::structures::DeviceIdentification
        createDeviceIdentification(unsigned int module, unsigned int type, const char *deviceRole,
                                   const char *deviceName, int priority);

    private:
        static InternalProtocol::Device
        createProtobufDevice(unsigned int module, unsigned int type, const char *deviceRole, const char *deviceName,
                             int priority);
    };

}