#include <bringauto/external_client/ExternalClient.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/external_client/connection/ConnectionState.hpp>

#include <bringauto/logging/Logger.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


namespace bringauto::external_client {

namespace ip = InternalProtocol;
using log = bringauto::logging::Logger;

ExternalClient::ExternalClient(std::shared_ptr <structures::GlobalContext> &context,
							   std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> &toExternalQueue)
		: context_ { context }, toExternalQueue_ { toExternalQueue } {
	fromExternalQueue_ = std::make_shared < structures::AtomicQueue < InternalProtocol::DeviceCommand >> ();
	reconnectQueue_ = std::make_shared<structures::AtomicQueue<std::reference_wrapper<connection::ExternalConnection>>>();
	fromExternalClientThread_ = std::thread(&ExternalClient::handleCommands, this);
};

void ExternalClient::handleCommands(){
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

void ExternalClient::handleCommand(const InternalProtocol::DeviceCommand &deviceCommand){
	const auto &device = deviceCommand.device();
	const auto &moduleNumber = device.module();
	if (not context_->statusAggregators.contains(moduleNumber)){
		log::logWarning("Module with module number {} does no exists", moduleNumber);
		return;
	}

	struct ::buffer commandBuffer {};
	const auto &commandData = deviceCommand.commanddata();
	if(allocate(&commandBuffer, commandData.size()) == NOT_OK) {
		log::logError("Could not allocate memory for command message");
		return;
	}
	std::memcpy(commandBuffer.data, commandData.c_str(), commandBuffer.size_in_bytes);
	auto deviceId = common_utils::ProtobufUtils::ParseDevice(device);

	int ret = context_->statusAggregators[moduleNumber]->update_command(commandBuffer, deviceId);
	if (ret != OK){
		log::logError("Update command failed with error code: {}", ret);
		return;
	}
	log::logInfo("Command on device {} was successfully updated", device.devicename());
}

void ExternalClient::destroy() {
	log::logInfo("External client stopped");
	for (auto& externalConnection : externalConnectionsList_) {
		externalConnection.endConnection(true);
	}
	fromExternalClientThread_.join();
}

void ExternalClient::run() {
	log::logInfo("External client started");
	initConnections();
	handleAggregatedMessages();
}

void ExternalClient::initConnections() {
	for(auto const &connection: context_->settings->externalConnectionSettingsList) {
		externalConnectionsList_.emplace_back(context_, connection, fromExternalQueue_, reconnectQueue_);
		auto &newConnection = externalConnectionsList_.back();
        newConnection.init(context_->settings->company, context_->settings->vehicleName);
		for(auto const &moduleNumber: connection.modules) {
			externalConnectionMap_.emplace(moduleNumber, newConnection);
		}
	}
}

void ExternalClient::handleAggregatedMessages() {
	while(not context_->ioContext.stopped()) {
		if (not reconnectQueue_->empty()) {
			startExternalConnectSequence(reconnectQueue_->front().get());
			reconnectQueue_->pop();
		}
		if(toExternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			continue;
		}
		log::logInfo("External client received aggregated status, number of aggregated statuses in queue {}", toExternalQueue_->size());
		auto& message = toExternalQueue_->front();
		sendStatus(message.devicestatus());
	}
}

void ExternalClient::sendStatus(const InternalProtocol::DeviceStatus& deviceStatus) {
	const auto &moduleNumber = deviceStatus.device().module();
    auto it = externalConnectionMap_.find(moduleNumber);
    if (it == externalConnectionMap_.end()) {
        log::logError("Module number {} not found in the map\n", moduleNumber);
        return;
    }

    auto &connection = it->second.get();
	if (connection.getState() != connection::CONNECTED) {
		connection.fillErrorAggregator(deviceStatus);
		if (connection.getState() == connection::NOT_INITIALIZED) {
			if (insideConnectSequence_) {
				log::logWarning("Status moved to error aggregator. Cannot initialize connect sequence, when different is running");
				return;
			}
			startExternalConnectSequence(connection);
		}
	} else {
		connection.sendStatus(deviceStatus);
        toExternalQueue_->pop();
	}
}

void ExternalClient::startExternalConnectSequence(connection::ExternalConnection& connection) {
    log::logInfo("Initializing new connection");
	insideConnectSequence_ = true;
	log::logDebug("External client is forcing aggregation on all modules");
	auto statusesLeft = connection.forceAggregationOnAllDevices();
	// maybe do it better way
	std::set<std::string> devices {};

	while (statusesLeft != 0) {
		if(toExternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			continue;
		}
		auto &status = toExternalQueue_->front().devicestatus();
		auto &device = status.device();
		if (connection.isModuleSupported(device.module())) {
			std::string deviceString = device.SerializeAsString();
			if(devices.find(deviceString) != devices.end()){
				log::logDebug("Cannot fill error aggregator for same device: {} {}", device.devicerole(), device.devicename());
				auto &message = toExternalQueue_->front();
				toExternalQueue_->pushAndNotify(message);
				toExternalQueue_->pop();
				continue;
			}
			log::logDebug("Filling error aggregator of device: {} {}", device.devicerole(), device.devicename());
			connection.fillErrorAggregator(status);
			devices.emplace(deviceString);
			statusesLeft--;
		} else {
			log::logDebug("Sending status inside connect sequence init");
			sendStatus(status); /// Send status from different connection, so it won't get lost. Shouldn't initialize bad recursion
		}
		toExternalQueue_->pop();
	}

	if (connection.initializeConnection() != 0) {
		boost::asio::deadline_timer timer(context_->ioContext);
		timer.expires_from_now(boost::posix_time::seconds(settings::reconnect_delay));
		timer.async_wait([this, &connection](const boost::system::error_code& error) {
			reconnectQueue_->push(connection);
		});
	}
	insideConnectSequence_ = false;
}

}