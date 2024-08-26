#include <bringauto/external_client/ExternalClient.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/external_client/connection/ConnectionState.hpp>

#include <bringauto/logging/Logger.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>



namespace bringauto::external_client {

namespace ip = InternalProtocol;
using log = bringauto::logging::Logger;

ExternalClient::ExternalClient(std::shared_ptr<structures::GlobalContext> &context,
							   structures::ModuleLibrary &moduleLibrary,
							   std::shared_ptr<structures::AtomicQueue<structures::InternalClientMessage>> &toExternalQueue):
		toExternalQueue_ { toExternalQueue },
		context_ { context },
		moduleLibrary_ { moduleLibrary },
		timer_ { context->ioContext } {
	fromExternalQueue_ = std::make_shared<structures::AtomicQueue<InternalProtocol::DeviceCommand >>();
	reconnectQueue_ =
			std::make_shared<structures::AtomicQueue<structures::ReconnectQueueItem >>();
	fromExternalClientThread_ = std::jthread(&ExternalClient::handleCommands, this);
}

void ExternalClient::handleCommands() {
	while(not context_->ioContext.stopped()) {
		if(fromExternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			continue;
		}
		log::logInfo("External client received command");

		auto &message = fromExternalQueue_->front();
		handleCommand(message);
		fromExternalQueue_->pop();
	}
}

void ExternalClient::handleCommand(const InternalProtocol::DeviceCommand &deviceCommand) {
	const auto &device = deviceCommand.device();
	const auto &moduleNumber = device.module();
	auto &statusAggregators = moduleLibrary_.statusAggregators;
	if(not statusAggregators.contains(moduleNumber)) {
		log::logWarning("Module with module number {} does no exists", moduleNumber);
		return;
	}

	const auto &commandData = deviceCommand.commanddata();
	if (commandData.empty()) {
		log::logWarning("Received empty command for device: {} {}", device.devicerole(), device.devicename());
		return;
	}
	auto &moduleLibraryHandler = moduleLibrary_.moduleLibraryHandlers.at(moduleNumber);
	auto commandBuffer = moduleLibraryHandler->constructBuffer(commandData.size());
	common_utils::ProtobufUtils::copyCommandToBuffer(deviceCommand, commandBuffer);


	auto deviceId = structures::DeviceIdentification(device);
	int ret = statusAggregators.at(moduleNumber)->update_command(commandBuffer, deviceId);
	if (ret == OK) {
		log::logInfo("Command for device {} was added to queue", device.devicename());
	}
}

void ExternalClient::destroy() {
	for(auto &externalConnection: externalConnectionsList_) {
		externalConnection.deinitializeConnection(true);
	}
	fromExternalClientThread_.join();
	log::logInfo("External client stopped");
}

void ExternalClient::run() {
	log::logInfo("External client started, constants used: reconnect_delay: {}, queue_timeout_length: {}, "
				 "immediate_disconnect_timeout: {}, status_response_timeout: {}",
				 settings::reconnect_delay, settings::queue_timeout_length.count(),
				 settings::immediate_disconnect_timeout.count(), settings::status_response_timeout.count());
	initConnections();
	handleAggregatedMessages();
}

void ExternalClient::initConnections() {
	for(auto const &connection: context_->settings->externalConnectionSettingsList) {
		externalConnectionsList_.emplace_back(context_, moduleLibrary_, connection, fromExternalQueue_,
											  reconnectQueue_);
		auto &newConnection = externalConnectionsList_.back();
		newConnection.init(context_->settings->company, context_->settings->vehicleName);
		for(auto const &moduleNumber: connection.modules) {
			externalConnectionMap_.emplace(moduleNumber, newConnection);
		}
	}
}

void ExternalClient::handleAggregatedMessages() {
	while(not context_->ioContext.stopped()) {
		if(not reconnectQueue_->empty()) {
			auto &reconnectItem = reconnectQueue_->front();
			auto &connection = reconnectItem.connection_.get();
			connection.deinitializeConnection(false);
			if(reconnectItem.reconnect) {
				startExternalConnectSequence(connection);
			} else {
				log::logInfo("External connection is disconnected from external server");
				connection.setNotInitialized();
			}
			reconnectQueue_->pop();
		}
		if(toExternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			continue;
		}
		log::logInfo("External client received aggregated status, number of aggregated statuses in queue {}",
					 toExternalQueue_->size());
		auto message = std::move(toExternalQueue_->front());
		toExternalQueue_->pop();
		if(not sendStatus(message)) {
			reconnectQueue_->waitForValueWithTimeout(std::chrono::seconds(settings::immediate_disconnect_timeout));
		}
	}
}

bool ExternalClient::sendStatus(const structures::InternalClientMessage &internalMessage) {
	auto &deviceStatus = internalMessage.getMessage().devicestatus();
	const auto &moduleNumber = deviceStatus.device().module();
	auto it = externalConnectionMap_.find(moduleNumber);
	if(it == externalConnectionMap_.end()) {
		log::logError("Module number {} not found in the map\n", moduleNumber);
		return true;
	}

	auto &connection = it->second.get();
	if(connection.getState() != connection::ConnectionState::CONNECTED) {
		connection.fillErrorAggregator(deviceStatus);
		//toExternalQueue_->pop();

		if(connection.getState() == connection::ConnectionState::NOT_INITIALIZED) {
			if(insideConnectSequence_) {
				log::logWarning(
						"Status moved to error aggregator. Cannot initialize connect sequence, when different is running");
				return true;
			}
			startExternalConnectSequence(connection);
		}
	} else {
		bool ret = true;
		if(internalMessage.disconnected()) {
			connection.sendStatus(deviceStatus, ExternalProtocol::Status_DeviceState_DISCONNECT);
			ret = false;
		} else {
			connection.sendStatus(deviceStatus);
		}
		//toExternalQueue_->pop();
		return ret;
	}

	return true;
}

void ExternalClient::startExternalConnectSequence(connection::ExternalConnection &connection) {
	log::logInfo("Initializing new connection");
	insideConnectSequence_ = true;

	while(not toExternalQueue_->empty()) {
		auto internalMessage = std::move(toExternalQueue_->front());
		toExternalQueue_->pop();
		const auto &deviceStatus = internalMessage.getMessage().devicestatus();
		if(connection.isModuleSupported(deviceStatus.device().module())) {
			connection.fillErrorAggregator(deviceStatus);
		} else {
			// Send status from different connection, so it won't get lost. Shouldn't initialize bad recursion
			sendStatus(internalMessage);
		}
	}

	log::logDebug("External client is forcing aggregation on all modules");
	auto connectedDevices = connection.getAllConnectedDevices();
	auto forcedDevices = connection.forceAggregationOnAllDevices(connectedDevices);

	while(not forcedDevices.empty() && not context_->ioContext.stopped()) {
		if(toExternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			continue;
		}
		const auto internalMessage = std::move(toExternalQueue_->front());
		toExternalQueue_->pop();

		const auto &deviceStatus = internalMessage.getMessage().devicestatus();
		const auto &device = deviceStatus.device();
		if(connection.isModuleSupported(device.module())) {
			auto deviceId = structures::DeviceIdentification(device);
			auto it = std::ranges::find(std::as_const(forcedDevices), deviceId);
			if(it == forcedDevices.cend()) {
				log::logDebug("Cannot fill error aggregator for same device: {} {}", device.devicerole(),
							  device.devicename());
				toExternalQueue_->pushAndNotify(internalMessage);
			} else {
				log::logDebug("Filling error aggregator of device: {} {}", device.devicerole(), device.devicename());
				connection.fillErrorAggregator(deviceStatus);
				forcedDevices.erase(it);
			}
		} else {
			log::logDebug("Sending status inside connect sequence init");
			// Send status from different connection, so it won't get lost. Shouldn't initialize bad recursion
			sendStatus(internalMessage);
		}
	}
	connection.fillErrorAggregatorWithNotAckedStatuses();

	if(connection.initializeConnection(connectedDevices) != 0 && not context_->ioContext.stopped()) {
		log::logDebug("Waiting for reconnect timer to expire");
		timer_.expires_from_now(boost::posix_time::seconds(settings::reconnect_delay));
		timer_.async_wait([this, &connection](const boost::system::error_code&) {
			reconnectQueue_->push(structures::ReconnectQueueItem(std::ref(connection), true));
			log::logDebug("Reconnect timer expired");
		});
	}
	insideConnectSequence_ = false;
}

}
