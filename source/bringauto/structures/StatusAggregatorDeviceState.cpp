#include <bringauto/structures/StatusAggregatorDeviceState.hpp>

#include <fleet_protocol/common_headers/memory_management.h>



namespace bringauto::structures {

StatusAggregatorDeviceState::StatusAggregatorDeviceState(
		std::shared_ptr<bringauto::structures::GlobalContext> &context,
		std::function<int(const structures::DeviceIdentification&)> fun, const structures::DeviceIdentification &deviceId,
		const bringauto::modules::Buffer command, const bringauto::modules::Buffer status
		// std::function<void(struct buffer *)> deallocateFun
		): status_ { status } {
	command_ = command;
	timer_ = std::make_unique<bringauto::structures::ThreadTimer>(context, fun, deviceId);
	// deallocateFun_ = deallocateFun;
	timer_->start();
}

// void StatusAggregatorDeviceState::deallocateStatus() {
// 	deallocateFun_(&status_);
// }

void StatusAggregatorDeviceState::setStatus(const bringauto::modules::Buffer &statusBuffer) {
	// deallocateStatus();
	status_ = statusBuffer;
}

const bringauto::modules::Buffer &StatusAggregatorDeviceState::getStatus() const {
	return status_;
}

void StatusAggregatorDeviceState::setStatusAndResetTimer(const bringauto::modules::Buffer &statusBuffer) {
	setStatus(statusBuffer);
	timer_->restart();
}

// void StatusAggregatorDeviceState::deallocateCommand() {
// 	deallocateFun_(&command_);
// }

void StatusAggregatorDeviceState::setCommand(const bringauto::modules::Buffer &commandBuffer) {
	// deallocateCommand();
	command_ = commandBuffer;
}

const bringauto::modules::Buffer &StatusAggregatorDeviceState::getCommand() const {
	return command_;
}

std::queue<struct bringauto::modules::Buffer> &StatusAggregatorDeviceState::aggregatedMessages() {
	return aggregatedMessages_;
}

}