#include <bringauto/structures/StatusAggregatorDeviceState.hpp>

#include <fleet_protocol/common_headers/memory_management.h>



namespace bringauto::structures {

StatusAggregatorDeviceState::StatusAggregatorDeviceState(
		std::shared_ptr<bringauto::structures::GlobalContext> &context,
		std::function<int(const struct ::device_identification)> fun, const device_identification &deviceId,
		const buffer command, const buffer status, std::function<void(struct buffer *)> deallocateFun): status_ { status } {
	command_ = command;
	timer_ = std::make_unique<bringauto::structures::ThreadTimer>(context, fun, deviceId);
	deallocateFun_ = deallocateFun;
	timer_->start();
}

void StatusAggregatorDeviceState::deallocateStatus() {
	deallocateFun_(&status_);
}

void StatusAggregatorDeviceState::setStatus(const buffer &statusBuffer) {
	deallocateStatus();
	status_ = statusBuffer;
}

const struct buffer &StatusAggregatorDeviceState::getStatus() const {
	return status_;
}

void StatusAggregatorDeviceState::setStatusAndResetTimer(const buffer &statusBuffer) {
	setStatus(statusBuffer);
	timer_->restart();
}

void StatusAggregatorDeviceState::deallocateCommand() {
	deallocateFun_(&command_);
}

void StatusAggregatorDeviceState::setCommand(const buffer &commandBuffer) {
	deallocateCommand();
	command_ = commandBuffer;
}

const struct buffer &StatusAggregatorDeviceState::getCommand() const {
	return command_;
}

std::queue<struct buffer> &StatusAggregatorDeviceState::getAggregatedMessages() {
	return aggregatedMessages_;
}

}