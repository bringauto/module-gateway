#include <bringauto/external_client/ExternalClient.hpp>
#include <bringauto/logging/Logger.hpp>
#include <bringauto/settings/Constants.hpp>



namespace bringauto::external_client {

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
		externalConnections_.push_back(connection::ExternalConnection(connection));
		auto &created_connection = externalConnections_.back();
		for(auto const &module: connection.modules) {
			connectionMap_.emplace(module, created_connection);
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

void ExternalClient::sendMessage(InternalProtocol::InternalClient &message) {

}

}