#include <bringauto/internal_server/InternalServer.hpp>
#include <bringauto/logging/Logger.hpp>

#include <algorithm>



namespace bringauto::internal_server {

void InternalServer::run() {
	logging::Logger::logInfo("Internal server started");
	boost::asio::ip::tcp::endpoint endpoint { boost::asio::ip::tcp::v4(), context_->settings->port };
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
			logging::Logger::logError("Error in addAsyncAccept(): {}", error.message());
			return;
		}
		boost::asio::socket_base::keep_alive option(true);
		connection->socket.set_option(option);
		logging::Logger::logInfo("accepted connection with Internal Client, "
								 "connection's ip address is {}",
								 connection->socket.remote_endpoint().address().to_string());
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
		if(connection->deviceId) {
			logging::Logger::logWarning(
					"Internal Client with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {})"
					" has been disconnected. Reason: {}", connection->deviceId->getModule(),
					connection->deviceId->getDeviceType(), connection->deviceId->getDeviceRole(),
					connection->deviceId->getDeviceName(), connection->deviceId->getPriority(),
					error.message());
		} else {
			logging::Logger::logWarning(
					"Internal Client with ip address {} has been disconnected. Reason: {}",
					connection->socket.remote_endpoint().address().to_string(), error.message());
		}
		std::lock_guard<std::mutex> lock(serverMutex_);
		removeConnFromMap(connection);
		return;
	}

	bool result = processBufferData(connection, bytesTransferred);
	if(result) {
		addAsyncReceive(connection);
	} else {
		std::lock_guard<std::mutex> lock(serverMutex_);
		removeConnFromMap(connection);
	}
}

bool InternalServer::processBufferData(
		const std::shared_ptr<structures::Connection> &connection,
		std::size_t bytesTransferred) {
	auto &completeMessageSize = connection->connContext.completeMessageSize;
	auto &completeMessage = connection->connContext.completeMessage;
	auto &buffer = connection->connContext.buffer;
	const uint8_t headerSize = 4;

	if(bytesTransferred < headerSize && completeMessageSize == 0) {
		logging::Logger::logError(
				"Error in processBufferData(...): Incomplete header received from Internal Client, "
				"connection's ip address is {}", connection->socket.remote_endpoint().address().to_string());
		return false;
	}

	std::size_t bytesLeft = bytesTransferred;
	auto dataBegin = buffer.begin();

	if(completeMessageSize == 0) {

		uint32_t size { 0 };

		std::copy(dataBegin, dataBegin + headerSize, (uint8_t *)&size);
		completeMessageSize = size;
		completeMessage.reserve(completeMessageSize);

		dataBegin = dataBegin + headerSize;
		bytesLeft -= headerSize;
	}
	const std::size_t messageBytesLeft = std::min(completeMessageSize - completeMessage.size(), bytesLeft);
	auto dataEnd = dataBegin + messageBytesLeft;

	std::copy(dataBegin, dataEnd, std::back_inserter(completeMessage));

	bytesLeft -= messageBytesLeft;
	if(bytesLeft) {
		logging::Logger::logError("Error in processBufferData(...): "
								  "Received extra bytes of data: {} from Internal Client, "
								  "connection's ip address is {}", bytesLeft,
								  connection->socket.remote_endpoint().address().to_string());
		return false;
	}

	if(completeMessageSize == completeMessage.size()) {
		if(!handleMessage(connection)) {
			return false;
		}
		completeMessage = {};
		completeMessageSize = 0;
	}
	return true;
}

bool InternalServer::handleMessage(const std::shared_ptr<structures::Connection> &connection) {

	InternalProtocol::InternalClient client {};
	if(!client.ParseFromArray(connection->connContext.completeMessage.data(),
							  connection->connContext.completeMessage.size())) {
		logging::Logger::logError(
				"Error in handleMessage(...): message received from Internal Client cannot be parsed, "
				"connection's ip address is {}", connection->socket.remote_endpoint().address().to_string());
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
		logging::Logger::logError(
				"Error in handleMessage(...): message received from Internal Client cannot be parsed, "
				"connection's ip address is {}", connection->socket.remote_endpoint().address().to_string());
		return false;
	}
	std::unique_lock<std::mutex> lk(connection->connectionMutex);
	connection->conditionVariable.wait_for(lk, settings::fleet_protocol_timeout_length,
										   [this, connection]() {
											   return connection->ready || context_->ioContext.stopped();
										   });
	if(!connection->ready) {
		logging::Logger::logError("Error in handleMessage(...): "
								  "Module Handler did not response to a message in time, "
								  "connection's ip address is {}",
								  connection->socket.remote_endpoint().address().to_string());
		return false;
	}
	return !context_->ioContext.stopped();
}

bool InternalServer::handleStatus(const std::shared_ptr<structures::Connection> &connection,
								  const InternalProtocol::InternalClient &client) {
	if(!connection->ready) {
		logging::Logger::logError("Error in handleStatus(...): "
								  "received status from Internal Client without being connected, "
								  "connection's ip address is {}",
								  connection->socket.remote_endpoint().address().to_string());
		return false;
	}
	fromInternalQueue_->pushAndNotify(structures::InternalClientMessage(false, client));
	connection->ready = false;
	return true;
}

bool InternalServer::handleConnection(const std::shared_ptr<structures::Connection> &connection,
									  const InternalProtocol::InternalClient &client) {
	std::lock_guard<std::mutex> lock(serverMutex_);
	if(connection->ready) {
		logging::Logger::logError("Error in handleConnection(...): "
								  "Internal Client is sending connect while already connected, "
								  "connection's ip address is {}",
								  connection->socket.remote_endpoint().address().to_string());
		return false;
	}

	auto deviceId = std::make_shared<structures::DeviceIdentification>(client.deviceconnect().device());
	auto existingConnection = findConnection(deviceId.get());

	if(!existingConnection) {
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

void InternalServer::connectNewDevice(const std::shared_ptr<structures::Connection> &connection,
									  const InternalProtocol::InternalClient &connect,
									  const std::shared_ptr<structures::DeviceIdentification> &deviceId) {
	connection->deviceId = deviceId;
	connectedDevices_.push_back(connection);
	fromInternalQueue_->pushAndNotify(structures::InternalClientMessage(false, connect));
	logging::Logger::logInfo(
			"Connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {}) "
			"has been added into the vector of active connections",
			connection->deviceId->getModule(),
			connection->deviceId->getDeviceType(), connection->deviceId->getDeviceRole(),
			connection->deviceId->getDeviceName(), connection->deviceId->getPriority());
}

void InternalServer::respondWithHigherPriorityConnected(const std::shared_ptr<structures::Connection> &connection,
														const InternalProtocol::InternalClient &connect,
														const std::shared_ptr<structures::DeviceIdentification> &deviceId) {
	auto message = common_utils::ProtobufUtils::createInternalServerConnectResponseMessage(
			connect.deviceconnect().device(),
			InternalProtocol::DeviceConnectResponse_ResponseType_HIGHER_PRIORITY_ALREADY_CONNECTED);
	logging::Logger::logInfo(
			"Connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {}) "
			"cannot be added same device is already connected with higher priority",
			deviceId->getModule(),
			deviceId->getDeviceType(), deviceId->getDeviceRole(),
			deviceId->getDeviceName(), deviceId->getPriority());
	sendResponse(connection, message);
}

void InternalServer::respondWithAlreadyConnected(const std::shared_ptr<structures::Connection> &connection,
												 const InternalProtocol::InternalClient &connect,
												 const std::shared_ptr<structures::DeviceIdentification> &deviceId) {
	auto message = common_utils::ProtobufUtils::createInternalServerConnectResponseMessage(
			connect.deviceconnect().device(),
			InternalProtocol::DeviceConnectResponse_ResponseType_ALREADY_CONNECTED);
	logging::Logger::logInfo(
			"Connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {}) "
			"cannot be added same device with same priority is already connected",
			deviceId->getModule(),
			deviceId->getDeviceType(), deviceId->getDeviceRole(),
			deviceId->getDeviceName(), deviceId->getPriority());
	sendResponse(connection, message);
}

void InternalServer::changeConnection(const std::shared_ptr<structures::Connection> &newConnection,
									  const InternalProtocol::InternalClient &connect,
									  const std::shared_ptr<structures::DeviceIdentification> &deviceId) {
	auto oldConnection = findConnection(deviceId.get());
	if(oldConnection) {
		removeConnFromMap(oldConnection);
	}
	connectNewDevice(newConnection, connect, deviceId);
	logging::Logger::logInfo("Old connection has been removed and replaced with new connection"
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
		logging::Logger::logError("Error in sendResponse(...): "
								  "Cannot write message header to Internal Client, "
								  "connection's ip address is {}",
								  connection->socket.remote_endpoint().address().to_string());
		return false;
	}
	try {
		const auto dataWSize = connection->socket.write_some(boost::asio::buffer(data));
		if(dataWSize != header) {
			logging::Logger::logError("Error in sendResponse(...): "
									  "Cannot write data to Internal Client, "
									  "connection's ip address is {}",
									  connection->socket.remote_endpoint().address().to_string());
			return false;
		}
	} catch(const boost::exception &e) {
		logging::Logger::logError("Error in sendResponse(...): "
								  "Cannot write data to Internal Client");
		return false;
	}
	return true;
}

void InternalServer::listenToQueue() {
	while(!context_->ioContext.stopped()) {
		if(!toInternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			auto message = toInternalQueue_->front();
			validateResponse(message);
			toInternalQueue_->pop();
		}
	}
}

void InternalServer::validateResponse(const InternalProtocol::InternalServer &message) {
	std::shared_ptr<structures::DeviceIdentification> deviceId;
	if(message.has_devicecommand()) {
		deviceId = std::make_shared<structures::DeviceIdentification>(message.devicecommand().device());
	}
	if(message.has_deviceconnectresponse()) {
		deviceId = std::make_shared<structures::DeviceIdentification>(message.deviceconnectresponse().device());
	}
	std::lock_guard<std::mutex> lock(serverMutex_);
	auto connection = findConnection(deviceId.get());

	if(connection && connection->deviceId->getPriority() == deviceId->getPriority()) {
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
	auto it = std::find_if(connectedDevices_.begin(), connectedDevices_.end(),
						   [&connection](const std::shared_ptr<structures::Connection> &toCompare) {
							   return connection->deviceId->isSame(toCompare->deviceId);
						   });

	if(it != connectedDevices_.end() && (*it)->deviceId->getPriority() == connection->deviceId->getPriority()) {
		connectedDevices_.erase(it);
		logging::Logger::logInfo(
				"connection with DeviceId(module: {}, deviceType: {}, deviceRole: {}, deviceName: {}, priority: {})"
				" has been closed and erased", connection->deviceId->getModule(),
				connection->deviceId->getDeviceType(), connection->deviceId->getDeviceRole(),
				connection->deviceId->getDeviceName(), connection->deviceId->getPriority());
		fromInternalQueue_->pushAndNotify(structures::InternalClientMessage(connection->deviceId->convertToCStruct()));
	}

}

std::shared_ptr<structures::Connection>
InternalServer::findConnection(structures::DeviceIdentification *deviceId) {
	std::shared_ptr<structures::Connection> connectionFound;
	auto it = std::find_if(connectedDevices_.begin(), connectedDevices_.end(),
						   [deviceId](auto toCompare) {
							   return deviceId->isSame(toCompare->deviceId);
						   });
	if(it != connectedDevices_.end()) {
		connectionFound = *it;
	}
	return connectionFound;
}

void InternalServer::destroy() {
	logging::Logger::logInfo("Internal server stopped");
	boost::system::error_code error {};
	acceptor_.cancel(error);
	acceptor_.close(error);
}


}