#include <bringauto/structures/StatusAggregatorDeviceState.hpp>

#include <memory_management.h>



namespace bringauto::structures {

StatusAggregatorDeviceState::StatusAggregatorDeviceState(
		std::shared_ptr<bringauto::structures::GlobalContext> &context,
		std::function<int(const struct ::device_identification)> fun, const device_identification &deviceId,
		const buffer command, const buffer status): status_ { status } {
	command_ = command;
	timer_ = std::make_unique<bringauto::structures::ThreadTimer>(context, fun, deviceId);
	timer_->start();
}

void StatusAggregatorDeviceState::deallocateStatus() {
	deallocate(&status_);
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

void StatusAggregatorDeviceState::deallocateCommand(){
	deallocate(&command_);
}

void StatusAggregatorDeviceState::setCommand(const buffer &commandBuffer){
	deallocateCommand();
	command_ = commandBuffer;
}

const struct buffer &StatusAggregatorDeviceState::getCommand() const {
	return command_;
}

std::queue<struct buffer> &StatusAggregatorDeviceState::getAggregatedMessages() {
	return aggregatedMessages;
}

}