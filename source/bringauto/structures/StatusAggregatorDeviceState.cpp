#include <bringauto/structures/StatusAggregatorDeviceState.hpp>

#include <fleet_protocol/common_headers/memory_management.h>



namespace bringauto::structures {

StatusAggregatorDeviceState::StatusAggregatorDeviceState(
		std::shared_ptr<bringauto::structures::GlobalContext> &context,
		std::function<int(const structures::DeviceIdentification&)> fun, const structures::DeviceIdentification &deviceId,
		const bringauto::modules::Buffer& command, const bringauto::modules::Buffer& status
		): status_ { status } {
	command_ = command;
	timer_ = std::make_unique<bringauto::structures::ThreadTimer>(context, fun, deviceId);
	timer_->start();
}

void StatusAggregatorDeviceState::setStatus(const bringauto::modules::Buffer &statusBuffer) {
	status_ = statusBuffer;
}

const bringauto::modules::Buffer &StatusAggregatorDeviceState::getStatus() const {
	return status_;
}

void StatusAggregatorDeviceState::setStatusAndResetTimer(const bringauto::modules::Buffer &statusBuffer) {
	setStatus(statusBuffer);
	timer_->restart();
}

void StatusAggregatorDeviceState::setCommand(const bringauto::modules::Buffer &commandBuffer) {
	command_ = commandBuffer;
}

const bringauto::modules::Buffer &StatusAggregatorDeviceState::getCommand() {
	if (!externalCommandQueue_.empty()) {
		command_ = externalCommandQueue_.front();
		externalCommandQueue_.pop();
	}
	return command_;
}

std::queue<struct bringauto::modules::Buffer> &StatusAggregatorDeviceState::aggregatedMessages() {
	return aggregatedMessages_;
}

int StatusAggregatorDeviceState::addExternalCommand(const bringauto::modules::Buffer &command) {
	externalCommandQueue_.push(command);
	if (externalCommandQueue_.size() > bringauto::settings::max_external_commands) {
		externalCommandQueue_.pop();
		return NOT_OK;
	}
	return OK;
}

}
