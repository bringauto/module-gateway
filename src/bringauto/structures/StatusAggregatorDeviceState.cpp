#include <bringauto/structures/StatusAggregatorDeviceState.hpp>



namespace bringauto::structures {

StatusAggregatorDeviceState::StatusAggregatorDeviceState(
		std::shared_ptr <bringauto::structures::GlobalContext> &context,
		std::function<int(const struct ::device_identification)> fun, const device_identification &deviceId,
		const buffer command, const buffer status) {
	this->status = status;
	this->command = command;
    // use constant instead of number 5
	timer = std::make_unique<bringauto::structures::ThreadTimer>(context, fun, deviceId, 5);
	timer->start();
}

void StatusAggregatorDeviceState::changeStatus(const buffer &buff) {
	deallocate(&status);
	status = buff;
	timer->restart();
}

}