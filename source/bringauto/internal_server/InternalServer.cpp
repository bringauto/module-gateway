#include <bringauto/internal_server/InternalServer.hpp>
#include <bringauto/settings/LoggerId.hpp>

#include <algorithm>



namespace bringauto::internal_server {

using log = settings::Logger;


void InternalServer::run() {
	log::logInfo("Internal server started, constants used: fleet_protocol_timeout_length: {}, queue_timeout_length: {}",
				 settings::fleet_protocol_timeout_length.count(),
				 settings::queue_timeout_length.count());
	const boost::asio::ip::tcp::endpoint endpoint { boost::asio::ip::tcp::v4(), context_->settings->port };
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();
	listeningThread = std::jthread([this]() { listenToQueue(); });
	addAsyncAccept();
}

void InternalServer::addAsyncAccept() {
	if(context_->ioContext.stopped()) {
		return;
	}
	auto connection = std::make_shared<structures::Connection>(context_->ioContext);
	acceptor_.async_accept(connection->socket, [this, connection](const boost::system::error_code &error) {
		if(error) {
			log::logError("Error in addAsyncAccept(): {}", error.message());
			return;
		}
		
		const boost::asio::socket_base::keep_alive keepAliveOption(true);
		connection->socket.set_option(keepAliveOption);
		const boost::asio::ip::tcp::no_delay noDelayOption(true);
		connection->socket.set_option(noDelayOption);

		log::logInfo("Accepted connection with Internal Client, "
					 "connection's ip address is {}",
					 connection->getRemoteEndpointAddress());
		addAsyncReceive(connection);
		addAsyncAccept();
	});
}

void InternalServer::addAsyncReceive(const std::shared_ptr<structures::Connection> &connection) {
	connection->socket.async_receive(
			boost::asio::buffer(connection->connContext.buffer),
			[this, connection](const boost::system::error_code &error,
							   const std::size_t bytesTransferred) {
				asyncReceiveHandler(connection, error, bytesTransferred);
			}
	);
}

void InternalServer::asyncReceiveHandler(
		const std::shared_ptr<structures::Connection> &connection,
		const boost::system::error_code &error,
		std::size_t bytesTransferred) {
	if(error) {
		if(connection->deviceId && error.value() == boost::asio::error::eof) {
			log::logWarning(
					"Internal Client with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {})"
					" has closed connection.", connection->deviceId->getModule(),
					connection->deviceId->getDeviceType(), connection->deviceId->getDeviceRole(),
					connection->deviceId->getDeviceName(), connection->deviceId->getPriority());
		} else if(connection->deviceId){
			log::logWarning(
					"Internal Client with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {})"
					" has been disconnected. Reason: {}", connection->deviceId->getModule(),
					connection->deviceId->getDeviceType(), connection->deviceId->getDeviceRole(),
					connection->deviceId->getDeviceName(), connection->deviceId->getPriority(),
					error.message());
		} else {
			log::logWarning(
					"Internal Client with ip address {} has been disconnected. Reason: {}",
					connection->getRemoteEndpointAddress(), error.message());
		}
		std::lock_guard<std::mutex> lock(serverMutex_);
		removeConnFromMap(connection);
		return;
	}

	const bool result = processBufferData(connection, bytesTransferred);
	if(result) {
		addAsyncReceive(connection);
	} else {
		std::lock_guard<std::mutex> lock(serverMutex_);
		removeConnFromMap(connection);
	}
}

bool InternalServer::processBufferData(
		const std::shared_ptr<structures::Connection> &connection,
		std::size_t bytesTransferred, std::size_t bufferOffset) {
	if(bytesTransferred < bufferOffset) {
		log::logError(
				"Error in processBufferData(...): bufferOffset: {} is greater than bytesTransferred: {}, "
				"Invalid bufferOffset: {} received from Internal Client, "
				"connection's ip address is {}", bufferOffset, bytesTransferred, 
				connection->getRemoteEndpointAddress());
		return false;
	}

	auto &completeMessageSize = connection->connContext.completeMessageSize;
	auto &completeMessage = connection->connContext.completeMessage;
	auto &buffer = connection->connContext.buffer;
	constexpr uint8_t headerSize = settings::header;

	if(bytesTransferred < headerSize && completeMessageSize == 0) {
		log::logError(
				"Error in processBufferData(...): Incomplete header received from Internal Client, "
				"connection's ip address is {}", connection->getRemoteEndpointAddress());
		return false;
	}

	std::size_t bytesLeft = bytesTransferred;
	auto dataBegin = buffer.begin() + bufferOffset;

	if(completeMessageSize == 0) {

		uint32_t size { 0 };

		std::copy_n(dataBegin, headerSize, reinterpret_cast<uint8_t *>(&size));
		completeMessageSize = size;

		dataBegin = dataBegin + headerSize;

		if(bytesLeft < headerSize) {
			log::logError(
					"Error in processBufferData(...): Incomplete header received from Internal Client, "
					"connection's ip address is {}", connection->getRemoteEndpointAddress());
			return false;
		}
		bytesLeft -= headerSize;
	}
	const auto messageBytesLeft = std::min(completeMessageSize - completeMessage.size(), bytesLeft);
	const auto dataEnd = dataBegin + messageBytesLeft;

	std::copy(dataBegin, dataEnd, std::back_inserter(completeMessage));

	if(completeMessageSize == completeMessage.size()) {
		if(!handleMessage(connection)) {
			return false;
		}
		completeMessage = {};
		completeMessageSize = 0;
	}

	if(bytesLeft < messageBytesLeft) {
		log::logError("Error in processBufferData(...): messageBytesLeft: {} is greater than bytesLeft: {}, "
					  "connection's ip address is {}", messageBytesLeft, bytesLeft,
					  connection->getRemoteEndpointAddress());
		return false;
	}
	bytesLeft -= messageBytesLeft;

	if(bytesTransferred < bytesLeft) {
		log::logError("Error in processBufferData(...): bytesLeft: {} is greater than bytesTransferred: {}, "
					  "connection's ip address is {}", bytesLeft, bytesTransferred,
					  connection->getRemoteEndpointAddress());
		return false;
	}
	if(bytesLeft && !processBufferData(connection, bytesLeft, bytesTransferred - bytesLeft)) {
		log::logError("Error in processBufferData(...): "
					  "Received extra invalid bytes of data: {} from Internal Client, "
					  "connection's ip address is {}", bytesLeft,
					  connection->getRemoteEndpointAddress());
		return false;
	}
	return true;
}

bool InternalServer::handleMessage(const std::shared_ptr<structures::Connection> &connection) {

	InternalProtocol::InternalClient client {};
	if(!client.ParseFromArray(connection->connContext.completeMessage.data(),
							  connection->connContext.completeMessage.size())) {
		log::logError(
				"Error in handleMessage(...): message received from Internal Client cannot be parsed, "
				"connection's ip address is {}", connection->getRemoteEndpointAddress());
		return false;
	}
	if(client.has_devicestatus()) {
		if(!handleStatus(connection, client)) {
			return false;
		}
	} else if(client.has_deviceconnect()) {
		if(!handleConnection(connection, client)) {
			return false;
		}
	} else {
		log::logError(
				"Error in handleMessage(...): message received from Internal Client cannot be parsed, "
				"connection's ip address is {}", connection->getRemoteEndpointAddress());
		return false;
	}
	std::unique_lock<std::mutex> lk(connection->connectionMutex);
	connection->conditionVariable.wait_for(lk, settings::fleet_protocol_timeout_length,
										   [this, connection]() {
											   return connection->ready || context_->ioContext.stopped();
										   });
	if(!connection->ready) {
		log::logError("Error in handleMessage(...): "
					  "Module Handler did not respond to a message in time, "
					  "connection's ip address is {}",
					  connection->getRemoteEndpointAddress());
		return false;
	}
	return !context_->ioContext.stopped();
}

bool InternalServer::handleStatus(const std::shared_ptr<structures::Connection> &connection,
								  const InternalProtocol::InternalClient &client) const {
	if(!connection->ready) {
		log::logError("Error in handleStatus(...): "
					  "received status from Internal Client without being connected, "
					  "connection's ip address is {}",
					  connection->getRemoteEndpointAddress());
		return false;
	}
	std::lock_guard<std::mutex> lk(connection->connectionMutex);
	fromInternalQueue_->pushAndNotify(structures::InternalClientMessage(false, client));
	connection->ready = false;
	return true;
}

bool InternalServer::handleConnection(const std::shared_ptr<structures::Connection> &connection,
									  const InternalProtocol::InternalClient &client) {
	std::lock_guard<std::mutex> lock(serverMutex_);
	if(connection->ready) {
		log::logError("Error in handleConnection(...): "
					  "Internal Client is sending a connect message while already connected, "
					  "connection's ip address is {}",
					  connection->getRemoteEndpointAddress());
		return false;
	}

	const structures::DeviceIdentification deviceId { client.deviceconnect().device() };
	const auto existingConnection = findConnection(deviceId);

	if(not existingConnection) {
		connectNewDevice(connection, client, deviceId);
	} else {
		if(client.deviceconnect().device().priority() == existingConnection->deviceId->getPriority()) {
			respondWithAlreadyConnected(connection, client, deviceId);
			return false;
		}
		if(client.deviceconnect().device().priority() > existingConnection->deviceId->getPriority()) {
			respondWithHigherPriorityConnected(connection, client, deviceId);
			return false;
		}
		if(client.deviceconnect().device().priority() < existingConnection->deviceId->getPriority()) {
			changeConnection(connection, client, deviceId);
		}
	}
	return true;
}

void InternalServer::handleDisconnect(const structures::DeviceIdentification& deviceId) {
	const auto connection = findConnection(deviceId);
	removeConnFromMap(connection);
}

void InternalServer::connectNewDevice(const std::shared_ptr<structures::Connection> &connection,
									  const InternalProtocol::InternalClient &connect,
									  const structures::DeviceIdentification &deviceId) {
	connection->deviceId = std::make_shared<structures::DeviceIdentification>(deviceId);
	connectedDevices_.push_back(connection);
	fromInternalQueue_->pushAndNotify(structures::InternalClientMessage(false, connect));
	log::logInfo(
			"Connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {}) "
			"has been added into the vector of active connections",
			connection->deviceId->getModule(),
			connection->deviceId->getDeviceType(), connection->deviceId->getDeviceRole(),
			connection->deviceId->getDeviceName(), connection->deviceId->getPriority());
}

void InternalServer::respondWithHigherPriorityConnected(const std::shared_ptr<structures::Connection> &connection,
														const InternalProtocol::InternalClient &connect,
														const structures::DeviceIdentification &deviceId) {
	const auto message = common_utils::ProtobufUtils::createInternalServerConnectResponseMessage(
			connect.deviceconnect().device(),
			InternalProtocol::DeviceConnectResponse_ResponseType_HIGHER_PRIORITY_ALREADY_CONNECTED);
	log::logInfo(
			"Connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {}) "
			"cannot be added, same device is already connected with higher priority",
			deviceId.getModule(),
			deviceId.getDeviceType(), deviceId.getDeviceRole(),
			deviceId.getDeviceName(), deviceId.getPriority());
	sendResponse(connection, message);
}

void InternalServer::respondWithAlreadyConnected(const std::shared_ptr<structures::Connection> &connection,
												 const InternalProtocol::InternalClient &connect,
												 const structures::DeviceIdentification &deviceId) {
	const auto message = common_utils::ProtobufUtils::createInternalServerConnectResponseMessage(
			connect.deviceconnect().device(),
			InternalProtocol::DeviceConnectResponse_ResponseType_ALREADY_CONNECTED);
	log::logInfo(
			"Connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {}) "
			"cannot be added, same device with same priority is already connected",
			deviceId.getModule(),
			deviceId.getDeviceType(), deviceId.getDeviceRole(),
			deviceId.getDeviceName(), deviceId.getPriority());
	sendResponse(connection, message);
}

void InternalServer::changeConnection(const std::shared_ptr<structures::Connection> &newConnection,
									  const InternalProtocol::InternalClient &connect,
									  const structures::DeviceIdentification &deviceId) {
	const auto oldConnection = findConnection(deviceId);
	if(oldConnection) {
		removeConnFromMap(oldConnection);
	}
	connectNewDevice(newConnection, connect, deviceId);
	log::logInfo("Old connection has been removed and replaced with new connection"
				 " with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {})",
				 newConnection->deviceId->getModule(),
				 newConnection->deviceId->getDeviceType(),
				 newConnection->deviceId->getDeviceRole(),
				 newConnection->deviceId->getDeviceName(),
				 newConnection->deviceId->getPriority());
}

bool InternalServer::sendResponse(const std::shared_ptr<structures::Connection> &connection,
								  const InternalProtocol::InternalServer &message) {
	if(not connection->socket.is_open()) {
		return false;
	}
	auto data = message.SerializeAsString();
	const uint32_t header = data.size();
	const auto headerWSize = connection->socket.write_some(boost::asio::buffer(&header, sizeof(uint32_t)));
	if(headerWSize != sizeof(uint32_t)) {
		log::logError("Error in sendResponse(...): "
					  "Cannot write message header to Internal Client, "
					  "connection's ip address is {}",
					  connection->getRemoteEndpointAddress());
		return false;
	}
	try {
		log::logDebug("Sending response to Internal Client, "
					  "connection's ip address is {}",
					  connection->getRemoteEndpointAddress());
		const auto dataWSize = connection->socket.write_some(boost::asio::buffer(data));
		if(dataWSize != header) {
			log::logError("Error in sendResponse(...): "
						  "Cannot write data to Internal Client, "
						  "connection's ip address is {}",
						  connection->getRemoteEndpointAddress());
			return false;
		}
	} catch(const boost::exception &) {
		log::logError("Error in sendResponse(...): "
					  "Cannot write data to Internal Client");
		return false;
	}
	return true;
}

void InternalServer::listenToQueue() {
	while(!context_->ioContext.stopped()) {
		if(!toInternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			auto &message = toInternalQueue_->front();
			if(message.disconnected()) {
				handleDisconnect(message.getDeviceId());
			} else {
				validateResponse(message.getMessage());
			}
			toInternalQueue_->pop();
		}
	}
}

void InternalServer::validateResponse(const InternalProtocol::InternalServer &message) {
	structures::DeviceIdentification deviceId { InternalProtocol::Device {} };
	if(message.has_devicecommand()) {
		deviceId = message.devicecommand().device();
	}
	if(message.has_deviceconnectresponse()) {
		deviceId = message.deviceconnectresponse().device();
	}
	std::lock_guard<std::mutex> lock(serverMutex_);
	const auto connection = findConnection(deviceId);

	if(connection && connection->deviceId->getPriority() == deviceId.getPriority()) {
		sendResponse(connection, message);
		{
			std::lock_guard<std::mutex> lk(connection->connectionMutex);
			connection->ready = true;
		}
		connection->conditionVariable.notify_one();
	}
}

void InternalServer::removeConnFromMap(const std::shared_ptr<structures::Connection> &connection) {
	boost::system::error_code error {};
	connection->socket.shutdown(boost::asio::socket_base::shutdown_both, error);
	connection->socket.close(error);
	if(connection->deviceId == nullptr) {
		return;
	}
	const auto it = std::find_if(connectedDevices_.begin(), connectedDevices_.end(),
								 [&connection](const std::shared_ptr<structures::Connection> &toCompare) {
									 return connection->deviceId->isSame(toCompare->deviceId);
								 });

	if(it != connectedDevices_.end() && (*it)->deviceId->getPriority() == connection->deviceId->getPriority()) {
		connectedDevices_.erase(it);
		log::logInfo(
				"connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {})"
				" has been closed and erased", connection->deviceId->getModule(),
				connection->deviceId->getDeviceType(), connection->deviceId->getDeviceRole(),
				connection->deviceId->getDeviceName(), connection->deviceId->getPriority());
		fromInternalQueue_->pushAndNotify(structures::InternalClientMessage(*connection->deviceId));
	}

}

std::shared_ptr<structures::Connection>
InternalServer::findConnection(const structures::DeviceIdentification &deviceId) {
	std::shared_ptr<structures::Connection> connectionFound {};
	const auto it = std::find_if(connectedDevices_.begin(), connectedDevices_.end(),
						   [&deviceId](const auto& toCompare) {
							   return deviceId.isSame(toCompare->deviceId);
						   });
	if(it != connectedDevices_.end()) {
		connectionFound = *it;
	}
	return connectionFound;
}

void InternalServer::destroy() {
	log::logInfo("Internal server stopped");
	boost::system::error_code error {};
	acceptor_.cancel(error);
	acceptor_.close(error);
}


}
