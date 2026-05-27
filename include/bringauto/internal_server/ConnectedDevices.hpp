#pragma once

#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/structures/Connection.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>

#include <InternalProtocol.pb.h>

#include <functional>
#include <memory>
#include <mutex>
#include <vector>



namespace bringauto::internal_server {

/**
 * Thread-safe registry of active device connections, protected by an internal mutex.
 * All compound operations (find + modify) are exposed as single atomic methods.
 */
class ConnectedDevices {
public:
	enum class RegisterResult { Connected, AlreadyConnected, HigherPriorityConnected };

	/**
	 * @brief Attempts to register a new connection for a device.
	 * If no existing connection for deviceId: registers and enqueues a connect message.
	 * If existing connection has same priority: returns AlreadyConnected (caller sends rejection).
	 * If existing has higher priority: returns HigherPriorityConnected (caller sends rejection).
	 * If existing has lower priority: closes old connection and replaces it with the new one.
	 */
	RegisterResult try_register(const std::shared_ptr<structures::Connection> &connection,
								const InternalProtocol::InternalClient &connect,
								const structures::DeviceIdentification &deviceId,
								structures::AtomicQueue<structures::InternalClientMessage> &queue);

	/**
	 * @brief Closes and removes a connection from the registry.
	 * Only removes if the connection matches by device id and priority.
	 */
	void remove(const std::shared_ptr<structures::Connection> &connection,
				structures::AtomicQueue<structures::InternalClientMessage> &queue);

	/**
	 * @brief Finds and removes the connection for the given device id.
	 */
	void remove_by_id(const structures::DeviceIdentification &deviceId,
					  structures::AtomicQueue<structures::InternalClientMessage> &queue);

	/**
	 * @brief Finds the connection matching deviceId and priority, sends message via send_fn,
	 * then sets connection->ready and notifies the waiting thread.
	 * Lock order: internal mutex_ then connection->connectionMutex.
	 */
	void deliver(const InternalProtocol::InternalServer &message,
				 const structures::DeviceIdentification &deviceId,
				 const std::function<bool(const std::shared_ptr<structures::Connection> &,
										 const InternalProtocol::InternalServer &)> &send_fn);

private:
	std::mutex mutex_;
	std::vector<std::shared_ptr<structures::Connection>> devices_;

	std::shared_ptr<structures::Connection> find_locked(const structures::DeviceIdentification &deviceId);

	void add_locked(const std::shared_ptr<structures::Connection> &connection,
					const InternalProtocol::InternalClient &connect,
					const structures::DeviceIdentification &deviceId,
					structures::AtomicQueue<structures::InternalClientMessage> &queue);

	void remove_locked(const std::shared_ptr<structures::Connection> &connection,
					   structures::AtomicQueue<structures::InternalClientMessage> &queue);
};

} // namespace bringauto::internal_server
