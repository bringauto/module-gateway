#include <bringauto/internal_server/ConnectedDevices.hpp>
#include <bringauto/settings/LoggerId.hpp>

#include <algorithm>



namespace bringauto::internal_server {

using log = settings::Logger;


std::shared_ptr<structures::Connection>
ConnectedDevices::find_locked(const structures::DeviceIdentification &deviceId) {
	const auto it = std::find_if(devices_.begin(), devices_.end(),
								 [&deviceId](const auto &toCompare) {
									 return deviceId.isSame(toCompare->deviceId);
								 });
	if(it != devices_.end()) {
		return *it;
	}
	return {};
}

void ConnectedDevices::add_locked(const std::shared_ptr<structures::Connection> &connection,
								  const InternalProtocol::InternalClient &connect,
								  const structures::DeviceIdentification &deviceId,
								  structures::AtomicQueue<structures::InternalClientMessage> &queue) {
	connection->deviceId = std::make_shared<structures::DeviceIdentification>(deviceId);
	devices_.push_back(connection);
	queue.pushAndNotify(structures::InternalClientMessage(false, connect));
	log::logInfo(
			"Connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {}) "
			"has been added into the vector of active connections",
			connection->deviceId->getModule(),
			connection->deviceId->getDeviceType(), connection->deviceId->getDeviceRole(),
			connection->deviceId->getDeviceName(), connection->deviceId->getPriority());
}

void ConnectedDevices::remove_locked(const std::shared_ptr<structures::Connection> &connection,
									 structures::AtomicQueue<structures::InternalClientMessage> &queue) {
	boost::system::error_code error {};
	connection->socket.shutdown(boost::asio::socket_base::shutdown_both, error);
	connection->socket.close(error);
	if(connection->deviceId == nullptr) {
		return;
	}
	const auto it = std::find_if(devices_.begin(), devices_.end(),
								 [&connection](const std::shared_ptr<structures::Connection> &toCompare) {
									 return connection->deviceId->isSame(toCompare->deviceId);
								 });
	if(it != devices_.end() && (*it)->deviceId->getPriority() == connection->deviceId->getPriority()) {
		devices_.erase(it);
		log::logInfo(
				"connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {})"
				" has been closed and erased", connection->deviceId->getModule(),
				connection->deviceId->getDeviceType(), connection->deviceId->getDeviceRole(),
				connection->deviceId->getDeviceName(), connection->deviceId->getPriority());
		queue.pushAndNotify(structures::InternalClientMessage(*connection->deviceId));
	}
}

ConnectedDevices::RegisterResult
ConnectedDevices::try_register(const std::shared_ptr<structures::Connection> &connection,
							   const InternalProtocol::InternalClient &connect,
							   const structures::DeviceIdentification &deviceId,
							   structures::AtomicQueue<structures::InternalClientMessage> &queue) {
	std::lock_guard lock(mutex_);
	const auto existing = find_locked(deviceId);
	if(!existing) {
		add_locked(connection, connect, deviceId, queue);
		return RegisterResult::Connected;
	}
	if(connect.deviceconnect().device().priority() == existing->deviceId->getPriority()) {
		return RegisterResult::AlreadyConnected;
	}
	if(connect.deviceconnect().device().priority() > existing->deviceId->getPriority()) {
		return RegisterResult::HigherPriorityConnected;
	}
	remove_locked(existing, queue);
	add_locked(connection, connect, deviceId, queue);
	log::logInfo("Old connection has been removed and replaced with new connection"
				 " with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {})",
				 connection->deviceId->getModule(),
				 connection->deviceId->getDeviceType(),
				 connection->deviceId->getDeviceRole(),
				 connection->deviceId->getDeviceName(),
				 connection->deviceId->getPriority());
	return RegisterResult::Connected;
}

void ConnectedDevices::remove(const std::shared_ptr<structures::Connection> &connection,
							  structures::AtomicQueue<structures::InternalClientMessage> &queue) {
	std::lock_guard lock(mutex_);
	remove_locked(connection, queue);
}

void ConnectedDevices::remove_by_id(const structures::DeviceIdentification &deviceId,
									structures::AtomicQueue<structures::InternalClientMessage> &queue) {
	std::lock_guard lock(mutex_);
	const auto connection = find_locked(deviceId);
	if(connection) {
		remove_locked(connection, queue);
	}
}

void ConnectedDevices::deliver(
		const InternalProtocol::InternalServer &message,
		const structures::DeviceIdentification &deviceId,
		const std::function<bool(const std::shared_ptr<structures::Connection> &,
								 const InternalProtocol::InternalServer &)> &send_fn) {
	std::lock_guard lock(mutex_);
	const auto connection = find_locked(deviceId);
	if(connection && connection->deviceId->getPriority() == deviceId.getPriority()) {
		send_fn(connection, message);
		{
			std::lock_guard lk(connection->connectionMutex);
			connection->ready = true;
		}
		connection->conditionVariable.notify_one();
	}
}

} // namespace bringauto::internal_server
