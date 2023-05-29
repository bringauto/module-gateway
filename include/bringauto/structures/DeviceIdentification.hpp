#pragma once

#include <InternalProtocol.pb.h>
#include <device_management.h>



namespace bringauto::structures {
class DeviceIdentification {
public:
	/**
	 * @brief construct object and fill params with values given in device
	 * @param device object holding values to be assigned to corresponding params
	 */
	explicit DeviceIdentification(const InternalProtocol::Device &device);
	/**
	 * @brief construct object and fill params with values given in device
	 * @param device object holding values to be assigned to corresponding params
	 */
	explicit DeviceIdentification(const device_identification& device);

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
	bool isSame(const std::shared_ptr<DeviceIdentification> &toCompare);


private:
	uint32_t module_;
	uint32_t deviceType_;
	std::string deviceRole_;
	std::string deviceName_;
	uint32_t priority_;
};
}