#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ThreadTimer.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>
#include <bringauto/modules/Buffer.hpp>
#include <fleet_protocol/common_headers/device_management.h>

#include <queue>
#include <memory>



namespace bringauto::structures {

/**
 * @brief State of the device in status aggregator
 */
class StatusAggregatorDeviceState {
public:

	StatusAggregatorDeviceState() = default;

	StatusAggregatorDeviceState(std::shared_ptr<GlobalContext> &context,
								std::function<int(const DeviceIdentification&)> fun,
								const DeviceIdentification &deviceId,
								const modules::Buffer& command, const modules::Buffer& status);

	/**
	 * @brief Deallocate and replace status buffer
	 *
	 * @param statusBuffer status data Buffer
	 */
	void setStatus(const modules::Buffer &statusBuffer);

	/**
	 * @brief Get status buffer
	 *
	 * @return const Buffer&
	 */
	[[nodiscard]] const modules::Buffer &getStatus() const;

	/**
	 * @brief Deallocate, replace data buffer and restart no aggregation timer
	 *
	 * @param statusBuffer status data Buffer
	 */
	void setStatusAndResetTimer(const modules::Buffer &statusBuffer);

	/**
	 * @brief Deallocate and replace command buffer
	 *
	 * @param commandBuffer command Buffer
	 */
	void setCommand(const modules::Buffer &commandBuffer);

	/**
	 * @brief Get command buffer
	 *
	 * @return const Buffer&
	 */
	[[nodiscard]] const modules::Buffer &getCommand();

	/**
	 * @brief Get aggregated messages queue
	 *
	 * @return std::queue<modules::Buffer>&
	 */
	[[nodiscard]] std::queue<modules::Buffer> &aggregatedMessages();

	/**
	 * @brief Add a command to the external command queue
	 *
	 * @param commandBuffer command Buffer
	 */
	int addExternalCommand(const modules::Buffer &commandBuffer);

private:
	std::unique_ptr<ThreadTimer> timer_ {};

	std::queue<modules::Buffer> aggregatedMessages_ {};

	modules::Buffer status_ {};

	modules::Buffer command_ {};

	std::queue<modules::Buffer> externalCommandQueue_ {};
};

}
