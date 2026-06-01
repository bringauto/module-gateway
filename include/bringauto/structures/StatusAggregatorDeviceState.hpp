#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ThreadTimer.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>
#include <bringauto/modules/Buffer.hpp>

#include <mutex>
#include <optional>
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
	 * @brief Sets the default command buffer
	 *
	 * @param commandBuffer command Buffer
	 */
	void setDefaultCommand(const modules::Buffer &commandBuffer);

	/**
	 * @brief Consumes and returns the most relevant command buffer.
	 * If there are external commands in the queue, they will be dequeued and returned.
	 * For push-only devices (forwardCommandOnReceive == OK) with an empty queue,
	 * returns std::nullopt — the caller must send an empty DeviceCommand to satisfy the
	 * InternalProtocol response requirement without forwarding anything to the vehicle.
	 * For pull devices, falls back to the default command buffer when the queue is empty.
	 * Commands dequeued from the external queue are removed from it.
	 *
	 * @return command buffer, or std::nullopt when push-only device has no pending command
	 */
	[[nodiscard]] std::optional<modules::Buffer> consumeCommand();

	/**
	 * @brief Get aggregated messages queue
	 *
	 * @return std::queue<modules::Buffer>&
	 */
	[[nodiscard]] std::queue<modules::Buffer> &aggregatedMessages();

	/**
	 * @brief Add a command to the queue of received commands from the external server.
	 * Commands are being retrieved by the getCommand method, which also removes them from the queue.
	 * If the queue is full, the oldest command will be removed.
	 *
	 * @param commandBuffer command Buffer
	 */
	int addExternalCommand(const modules::Buffer &commandBuffer);

	/**
	 * @brief Mark this device as requiring immediate command forwarding.
	 * Called once at device registration based on the module's forwardCommandOnReceive response.
	 */
	void enableImmediateCommandForwarding() noexcept;

	/**
	 * @brief Returns true if this device requires commands to be forwarded immediately upon receipt.
	 */
	[[nodiscard]] bool isForwardCommandImmediately() const noexcept;

private:
	std::unique_ptr<ThreadTimer> timer_ {};

	std::queue<modules::Buffer> aggregatedMessages_ {};

	modules::Buffer status_ {};

	modules::Buffer defaultCommand_ {};

	std::unique_ptr<std::mutex> externalCommandMutex_ { std::make_unique<std::mutex>() };

	std::queue<modules::Buffer> externalCommandQueue_ {};

	bool forwardCommandImmediately_ { false };
};

}
