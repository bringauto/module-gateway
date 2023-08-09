#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ThreadTimer.hpp>
#include <memory_management.h>
#include <device_management.h>

#include <queue>
#include <memory>



namespace bringauto::structures {

struct StatusAggregatorDeviceState {

	StatusAggregatorDeviceState() = default;

	StatusAggregatorDeviceState(std::shared_ptr <bringauto::structures::GlobalContext> &context,
								std::function<int(const struct ::device_identification)> fun,
								const device_identification &deviceId, const buffer command, const buffer status);

	void changeStatus(const buffer &buff);

	std::queue<struct buffer> aggregatedMessages;

	struct buffer status;

	struct buffer command;

private:
	std::unique_ptr <bringauto::structures::ThreadTimer> timer {};
};

}