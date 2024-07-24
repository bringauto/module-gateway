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

	StatusAggregatorDeviceState(std::shared_ptr<bringauto::structures::GlobalContext> &context,
								std::function<int(const structures::DeviceIdentification&)> fun,
								const structures::DeviceIdentification &deviceId,
								const bringauto::modules::Buffer& command, const bringauto::modules::Buffer& status);

	/**
	 * @brief Deallocate and replace status buffer
	 *
	 * @param statusBuffer status data Buffer
	 */
	void setStatus(const bringauto::modules::Buffer &statusBuffer);

	/**
	 * @brief Get status buffer
	 *
	 * @return const Buffer&
	 */
	[[nodiscard]] const bringauto::modules::Buffer &getStatus() const;

	/**
	 * @brief Deallocate, replace data buffer and restart no aggregation timer
	 *
	 * @param statusBuffer status data Buffer
	 */
	void setStatusAndResetTimer(const bringauto::modules::Buffer &statusBuffer);

	/**
	 * @brief Deallocate and replace command buffer
	 *
	 * @param commandBuffer command Buffer
	 */
	void setCommand(const bringauto::modules::Buffer &commandBuffer);

	/**
	 * @brief Get command buffer
	 *
	 * @return const Buffer&
	 */
	[[nodiscard]] const bringauto::modules::Buffer &getCommand();

	/**
	 * @brief Get aggregated messages queue
	 *
	 * @return std::queue<bringauto::modules::Buffer>&
	 */
	[[nodiscard]] std::queue<bringauto::modules::Buffer> &aggregatedMessages();

	/**
	 * @brief Add a command to the external command queue
	 *
	 * @param commandBuffer command Buffer
	 */
	int addExternalCommand(const bringauto::modules::Buffer &commandBuffer);

private:
	std::unique_ptr<bringauto::structures::ThreadTimer> timer_ {};

	std::queue<bringauto::modules::Buffer> aggregatedMessages_ {};

	bringauto::modules::Buffer status_ {};

	bringauto::modules::Buffer command_ {};

	std::queue<bringauto::modules::Buffer> externalCommandQueue_ {};
};

}