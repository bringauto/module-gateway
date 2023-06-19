#include <bringauto/external_client/ExternalClient.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/utils/utils.hpp>



namespace bringauto::external_client {

namespace ip = InternalProtocol;
using log = bringauto::logging::Logger;

ExternalClient::ExternalClient(std::shared_ptr <structures::GlobalContext> &context,
							   std::shared_ptr <structures::AtomicQueue<InternalProtocol::InternalClient>> &toExternalQueue)
		: context_ { context }, toExternalQueue_ { toExternalQueue } {
	fromExternalQueue_ = std::make_shared < structures::AtomicQueue < InternalProtocol::DeviceCommand >> ();
    fromExternalClientThread_ = std::thread(&ExternalClient::handleCommands, this);
};

void ExternalClient::destroy() {
	log::logInfo("External client stopped");
    fromExternalClientThread_.join();
}

void ExternalClient::run() {
	log::logInfo("External client started");
	initConnections();
	handleAggregatedMessages();
}

void ExternalClient::initConnections() {
	for(auto const &connection: context_->settings->externalConnectionSettingsList) {
		externalConnectionsList_.emplace_back(context_, connection, context_->settings->company, context_->settings->vehicleName, fromExternalQueue_);
		auto &created_connection = externalConnectionsList_.back();
		for(auto const &moduleNumber: connection.modules) {
			externalConnectionMap_.emplace(moduleNumber, created_connection);
		}
	}
}

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
    if(allocate(&commandBuffer, commandData.size() + 1) == NOT_OK) {
        log::logError("Could not allocate memory for command message");
        return;
    }
    strcpy(static_cast<char *>(commandBuffer.data), commandData.c_str());
    auto deviceId = utils::mapToDeviceId(device);

    int ret = context_->statusAggregators[moduleNumber]->update_command(commandBuffer, deviceId);
    if (ret != NOT_OK){
        log::logError("Update command failed with error code: {}", ret);
        return;
    }
    log::logInfo("Command on device {} was succesfully updated", device.devicename());
}

void ExternalClient::handleAggregatedMessages() {
	while(not context_->ioContext.stopped()) {
		if(toExternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			continue;
		}
		log::logInfo("External client received aggregated status, number of aggregated statuses in queue {}", toExternalQueue_->size());
		auto &message = toExternalQueue_->front();
		sendStatus(message);
		toExternalQueue_->pop();
	}
}

void ExternalClient::sendStatus(ip::InternalClient &message) {
	const auto &moduleNumber = message.devicestatus().device().module();
	auto &connection = externalConnectionMap_.at(moduleNumber).get();
	connection.sendStatus(message.devicestatus()); // TODO device state
}

}