#include <bringauto/external_client/ExternalClient.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/settings/Constants.hpp>



namespace bringauto::external_client {

namespace ip = InternalProtocol;
using log = bringauto::logging::Logger;

void ExternalClient::destroy() {
	log::logInfo("External client stopped");
}

void ExternalClient::run() {
	log::logInfo("External client started");
	initConnections();
	handleAggregatedMessages();
}

void ExternalClient::initConnections() {
	for(auto const &connection: context_->settings->externalConnectionSettingsList) {
		externalConnectionsVec_.push_back(connection::ExternalConnection(connection));
		auto &created_connection = externalConnectionsVec_.back();
		for(auto const &moduleNumber: connection.modules) {
			externalConnectionMap_.emplace(moduleNumber, created_connection);
		}
	}
}

void ExternalClient::handleAggregatedMessages() {
	while(not context_->ioContext.stopped()) {
		if(toExternalQueue_->waitForValueWithTimeout(settings::queue_timeout_length)) {
			continue;
		}
		auto &message = toExternalQueue_->front();
		log::logInfo("External client has new message, number of messages in queue {}", toExternalQueue_->size());
		sendMessage(message);
		toExternalQueue_->pop();
	}
}

void ExternalClient::sendMessage(ip::InternalClient &message) {
	const auto &moduleNumber = message.devicestatus().device().module();
    auto &connection = externalConnectionMap_.at(moduleNumber).get();
}

}