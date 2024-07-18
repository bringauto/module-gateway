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
								const bringauto::modules::Buffer command, const bringauto::modules::Buffer status);
								// std::function<void(struct buffer *)> dealloc);

	/**
	 * @brief Deallocate and replace status buffer
	 *
	 * @param statusBuffer status data buffer
	 */
	void setStatus(const bringauto::modules::Buffer &statusBuffer);

	/**
	 * @brief Get status buffer
	 *
	 * @return const struct buffer&
	 */
	[[nodiscard]] const bringauto::modules::Buffer &getStatus() const;

	/**
	 * @brief Deallocate status buffer
	 */
	// void deallocateStatus();

	/**
	 * @brief Deallocate, replace data buffer and restart no aggregation timer
	 *
	 * @param statusBuffer status data buffer
	 */
	void setStatusAndResetTimer(const bringauto::modules::Buffer &statusBuffer);

	/**
	 * @brief Deallocate and replace command buffer
	 *
	 * @param commandBuffer command buffer
	 */
	void setCommand(const bringauto::modules::Buffer &commandBuffer);

	/**
	 * @brief Get command buffer
	 *
	 * @return const struct buffer&
	 */
	[[nodiscard]] const bringauto::modules::Buffer &getCommand() const;

	/**
	 * @brief Deallocate command buffer
	 */
	// void deallocateCommand();

	/**
	 * @brief Get aggregated messages queue
	 *
	 * @return std::queue<struct buffer>&
	 */
	[[nodiscard]] std::queue<bringauto::modules::Buffer> &aggregatedMessages();

private:
	std::unique_ptr<bringauto::structures::ThreadTimer> timer_ {};

	std::queue<bringauto::modules::Buffer> aggregatedMessages_;

	// std::function<void(struct buffer *)> deallocateFun_;

	bringauto::modules::Buffer status_;

	bringauto::modules::Buffer command_;
};

}