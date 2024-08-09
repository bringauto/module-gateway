#include <bringauto/structures/StatusAggregatorDeviceState.hpp>

#include <fleet_protocol/common_headers/memory_management.h>



namespace bringauto::structures {

StatusAggregatorDeviceState::StatusAggregatorDeviceState(
		std::shared_ptr<GlobalContext> &context,
		std::function<int(const DeviceIdentification&)> fun, const DeviceIdentification &deviceId,
		const modules::Buffer& command, const modules::Buffer& status
		): status_ { status } {
	defaultCommand_ = command;
	timer_ = std::make_unique<ThreadTimer>(context, fun, deviceId);
	timer_->start();
}

void StatusAggregatorDeviceState::setStatus(const modules::Buffer &statusBuffer) {
	status_ = statusBuffer;
}

const modules::Buffer &StatusAggregatorDeviceState::getStatus() const {
	return status_;
}

void StatusAggregatorDeviceState::setStatusAndResetTimer(const modules::Buffer &statusBuffer) {
	setStatus(statusBuffer);
	timer_->restart();
}

void StatusAggregatorDeviceState::setDefaultCommand(const modules::Buffer &commandBuffer) {
	defaultCommand_ = commandBuffer;
}

const modules::Buffer &StatusAggregatorDeviceState::getCommand() {
	if (!externalCommandQueue_.empty()) {
		defaultCommand_ = externalCommandQueue_.front();
		externalCommandQueue_.pop();
	}
	return defaultCommand_;
}

std::queue<struct modules::Buffer> &StatusAggregatorDeviceState::aggregatedMessages() {
	return aggregatedMessages_;
}

int StatusAggregatorDeviceState::addExternalCommand(const modules::Buffer &command) {
	externalCommandQueue_.push(command);
	if (externalCommandQueue_.size() > settings::max_external_commands) {
		externalCommandQueue_.pop();
		return NOT_OK;
	}
	return OK;
}

}
