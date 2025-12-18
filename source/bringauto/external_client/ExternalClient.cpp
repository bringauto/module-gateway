#include <bringauto/external_client/ExternalClient.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/external_client/connection/ConnectionState.hpp>
#include <bringauto/external_client/connection/communication/MqttCommunication.hpp>
#include <bringauto/external_client/connection/communication/DummyCommunication.hpp>
#include <bringauto/settings/LoggerId.hpp>

#include <fleet_protocol/common_headers/general_error_codes.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "bringauto/external_client/connection/communication/QuicCommunication.hpp"


namespace bringauto::external_client {

namespace ip = InternalProtocol;

ExternalClient::ExternalClient(const std::shared_ptr<structures::GlobalContext> &context,
							   structures::ModuleLibrary &moduleLibrary,
							   const std::shared_ptr<structures::AtomicQueue<structures::InternalClientMessage>> &toExternalQueue):
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
		settings::Logger::logInfo("External client received command");

		auto &message = fromExternalQueue_->front();
		handleCommand(message);
		fromExternalQueue_->pop();
	}
}

void ExternalClient::handleCommand(const InternalProtocol::DeviceCommand &deviceCommand) {
	const auto &device = deviceCommand.device();
	const auto &moduleNumber = device.module();
	auto &statusAggregators = moduleLibrary_.statusAggregators;
	const auto it = statusAggregators.find(moduleNumber);
	if (it == statusAggregators.end()) {
		settings::Logger::logWarning("Module with module number {} does no exists", static_cast<int>(moduleNumber));
		return;
	}

	const auto &commandData = deviceCommand.commanddata();
	if (commandData.empty()) {
		settings::Logger::logWarning("Received empty command for device: {} {}", device.devicerole(), device.devicename());
		return;
	}
	const auto &moduleLibraryHandler = moduleLibrary_.moduleLibraryHandlers.at(moduleNumber);
	const auto commandBuffer = moduleLibraryHandler->constructBuffer(commandData.size());
	common_utils::ProtobufUtils::copyCommandToBuffer(deviceCommand, commandBuffer);


	const auto deviceId = structures::DeviceIdentification(device);
	int ret = it->second->update_command(commandBuffer, deviceId);
	if (ret == OK) {
		settings::Logger::logInfo("Command for device {} was added to queue", device.devicename());
	}
}

void ExternalClient::destroy() {
	for(auto &externalConnection: externalConnectionsList_) {
		externalConnection.deinitializeConnection(true);
	}
	fromExternalClientThread_.join();
	settings::Logger::logInfo("External client stopped");
}

void ExternalClient::run() {
	settings::Logger::logInfo("External client started, constants used: reconnect_delay: {}, queue_timeout_length: {}, "
				 "immediate_disconnect_timeout: {}, status_response_timeout: {}",
				 settings::reconnect_delay, settings::queue_timeout_length.count(),
				 settings::immediate_disconnect_timeout.count(), settings::status_response_timeout.count());
	initConnections();
	handleAggregatedMessages();
}

void ExternalClient::initConnections() {
	for(auto const &connectionSettings: context_->settings->externalConnectionSettingsList) {
		externalConnectionsList_.emplace_back(context_, moduleLibrary_, connectionSettings, fromExternalQueue_,
											  reconnectQueue_);
		auto &newConnection = externalConnectionsList_.back();
		std::shared_ptr<connection::communication::ICommunicationChannel> communicationChannel;

		switch(connectionSettings.protocolType) {
			case structures::ProtocolType::MQTT:
				communicationChannel = std::make_shared<connection::communication::MqttCommunication>(
					connectionSettings, context_->settings->company, context_->settings->vehicleName
				);
				break;
			case structures::ProtocolType::QUIC:
				communicationChannel = std::make_shared<connection::communication::QuicCommunication>(
					connectionSettings, context_->settings->company, context_->settings->vehicleName
				);
				break;
			case structures::ProtocolType::DUMMY:
				communicationChannel = std::make_shared<connection::communication::DummyCommunication>(
					connectionSettings
				);
				break;
			case structures::ProtocolType::INVALID:
			default:
				settings::Logger::logError("Invalid external communication protocol type");
				throw std::invalid_argument("Invalid external communication protocol type");
		}

		newConnection.init(communicationChannel);
		for(auto const &moduleNumber: connectionSettings.modules) {
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
				settings::Logger::logInfo("External connection is disconnected from external server");
				connection.setNotInitialized();
			}
			reconnectQueue_->pop();
		}
		if(toExternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			continue;
		}
		settings::Logger::logInfo("External client received aggregated status, number of aggregated statuses in queue {}",
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
	const auto it = externalConnectionMap_.find(moduleNumber);
	if(it == externalConnectionMap_.end()) {
		settings::Logger::logError("Module number {} not found in the map\n", static_cast<int>(moduleNumber));
		return true;
	}

	auto &connection = it->second.get();
	if(connection.getState() != connection::ConnectionState::CONNECTED) {
		connection.fillErrorAggregator(deviceStatus);

		if(connection.getState() == connection::ConnectionState::NOT_INITIALIZED) {
			if(insideConnectSequence_) {
				settings::Logger::logWarning(
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
		return ret;
	}

	return true;
}

void ExternalClient::startExternalConnectSequence(connection::ExternalConnection &connection) {
	settings::Logger::logInfo("Initializing new connection");
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

	settings::Logger::logDebug("External client is forcing aggregation on all modules");
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
				settings::Logger::logDebug("Cannot fill error aggregator for same device: {} {}", device.devicerole(),
							  device.devicename());
				toExternalQueue_->pushAndNotify(internalMessage);
			} else {
				settings::Logger::logDebug("Filling error aggregator of device: {} {}", device.devicerole(), device.devicename());
				connection.fillErrorAggregator(deviceStatus);
				forcedDevices.erase(it);
			}
		} else {
			settings::Logger::logDebug("Sending status inside connect sequence init");
			// Send status from different connection, so it won't get lost. Shouldn't initialize bad recursion
			sendStatus(internalMessage);
		}
	}
	connection.fillErrorAggregatorWithNotAckedStatuses();

	if(connection.initializeConnection(connectedDevices) != 0 && not context_->ioContext.stopped()) {
		settings::Logger::logDebug("Waiting for reconnect timer to expire");
		timer_.expires_from_now(boost::posix_time::seconds(settings::reconnect_delay));
		timer_.async_wait([this, &connection](const boost::system::error_code&) {
			reconnectQueue_->push(structures::ReconnectQueueItem(std::ref(connection), true));
			settings::Logger::logDebug("Reconnect timer expired");
		});
	}
	insideConnectSequence_ = false;
}

}
