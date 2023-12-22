#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ThreadTimer.hpp>
#include <device_management.h>

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
								std::function<int(const struct ::device_identification)> fun,
								const device_identification &deviceId, const buffer command, const buffer status, std::function<void(struct buffer *)> dealloc);

	/**
	 * @brief Deallocate and replace status buffer
	 *
	 * @param statusBuffer status data buffer
	 */
	void setStatus(const buffer &statusBuffer);

	/**
	 * @brief Get status buffer
	 *
	 * @return const struct buffer&
	 */
	[[nodiscard]] const struct buffer &getStatus() const;

	/**
	 * @brief Deallocate status buffer
	 */
	void deallocateStatus();

	/**
	 * @brief Deallocate, replace data buffer and restart no aggregation timer
	 *
	 * @param statusBuffer status data buffer
	 */
	void setStatusAndResetTimer(const buffer &statusBuffer);

	/**
	 * @brief Deallocate and replace command buffer
	 *
	 * @param commandBuffer command buffer
	 */
	void setCommand(const buffer &commandBuffer);

	/**
	 * @brief Get command buffer
	 *
	 * @return const struct buffer&
	 */
	[[nodiscard]] const struct buffer &getCommand() const;

	/**
	 * @brief Deallocate command buffer
	 */
	void deallocateCommand();

	/**
	 * @brief Get aggregated messages queue
	 *
	 * @return std::queue<struct buffer>&
	 */
	[[nodiscard]] std::queue<struct buffer> &getAggregatedMessages();

private:
	std::unique_ptr<bringauto::structures::ThreadTimer> timer_ {};

	std::queue<struct buffer> aggregatedMessages_;

	std::function<void(struct buffer *)> deallocateFun_;

	struct buffer status_;

	struct buffer command_;
};

}