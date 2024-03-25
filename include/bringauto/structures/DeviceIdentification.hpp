#pragma once

#include <InternalProtocol.pb.h>
#include <fleet_protocol/common_headers/device_management.h>



namespace bringauto::structures {
class DeviceIdentification {
public:
	/**
	 * @brief Construct object and fill params with values given in device
	 * @param device object holding values to be assigned to corresponding params
	 */
	explicit DeviceIdentification(const InternalProtocol::Device &device);

	/**
	 * @brief Construct object and fill params with values given in device
	 * @param device object holding values to be assigned to corresponding params
	 */
	explicit DeviceIdentification(const device_identification &device);

	/**
	 * @brief get value of module_
	 * @return value of module_
	 */
	[[nodiscard]] uint32_t getModule() const;

	/**
	 * @brief get value of deviceType_
	 * @return value of deviceType_
	 */
	[[nodiscard]] uint32_t getDeviceType() const;

	/**
	 * @brief get deviceRole_ string
	 * @return deviceRole_ string
	 */
	[[nodiscard]] const std::string &getDeviceRole() const;

	/**
	 * @brief get deviceName_ string
	 * @return deviceName_ string
	 */
	[[nodiscard]] const std::string &getDeviceName() const;

	/**
	 * @brief get priority value
	 * @return priorirt value
	 */
	[[nodiscard]] uint32_t getPriority() const;

	/**
	 * @brief Checks if all parameters except priority are equal.
	 * @param toCompare device used for comparison
	 * @return true if all parameters outside of priority are equal
	 */
	[[nodiscard]] bool isSame(const std::shared_ptr <DeviceIdentification> &toCompare) const;

	/**
	 * @brief Assigns values from device to this object
	 * @param device object holding values to be assigned to corresponding params
	 */
	DeviceIdentification& operator=(const InternalProtocol::Device &device);

	/**
	 * @brief Checks if all parameters except priority are equal.
	 * @param deviceId device used for comparison
	 * @return true if all parameters outside of priority are equal
	 */
	bool operator==(const DeviceIdentification &deviceId) const;

	/**
	 * @brief Create device_identification struct from this object
	 *
	 * @return device_identification
	 */
	[[nodiscard]] device_identification convertToCStruct() const;

	/**
	 * @brief Create a Internal protocol protobuf device object
	 *
	 * @return InternalProtocol::Device
	 */
	[[nodiscard]] InternalProtocol::Device convertToIPDevice() const;

private:
	/// Module number
	uint32_t module_;
	/// Device type
	uint32_t deviceType_;
	/// Role of the device
	std::string deviceRole_;
	/// Name of the device
	std::string deviceName_;
	/// Priority of the device
	uint32_t priority_;
};
}