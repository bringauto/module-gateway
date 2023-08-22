#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ThreadTimer.hpp>
#include <device_management.h>

#include <queue>
#include <memory>



namespace bringauto::structures {

class StatusAggregatorDeviceState {
public:

	StatusAggregatorDeviceState() = default;

	StatusAggregatorDeviceState(std::shared_ptr<bringauto::structures::GlobalContext> &context,
								std::function<int(const struct ::device_identification)> fun,
								const device_identification &deviceId, const buffer command, const buffer status);

	void setStatus(const buffer &statusBuffer);

	const struct buffer &getStatus();

	void deallocateStatus();

	void setStatusAndResetTimer(const buffer &statusBuffer);

	std::queue<struct buffer> &getAggregatedMessages();

	struct buffer command;

private:
	std::unique_ptr<bringauto::structures::ThreadTimer> timer_ {};

	std::queue<struct buffer> aggregatedMessages;

	struct buffer status_;
};

}