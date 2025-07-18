#include <bringauto/external_client/connection/communication/DummyCommunication.hpp>
#include <bringauto/settings/LoggerId.hpp>



namespace bringauto::external_client::connection::communication {

DummyCommunication::DummyCommunication(const structures::ExternalConnectionSettings &settings) : ICommunicationChannel(settings) {
    settings::Logger::logDebug("Creating DummyCommunication");
}

DummyCommunication::~DummyCommunication() {
    closeConnection();
}

void DummyCommunication::initializeConnection() {
    settings::Logger::logDebug("Initializing DummyCommunication connection");
    isConnected_ = true;
}

bool DummyCommunication::sendMessage(ExternalProtocol::ExternalClient *message) {
    if (!isConnected_) {
        settings::Logger::logDebug("Failed sending message, DummyCommunication is not connected");
        return false;
    }

    settings::Logger::logDebug("Sending message in DummyCommunication");
    return true;
}

std::shared_ptr<ExternalProtocol::ExternalServer> DummyCommunication::receiveMessage() {
    settings::Logger::logDebug("Receiving message in DummyCommunication");
    return nullptr;
}

void DummyCommunication::closeConnection() {
    settings::Logger::logDebug("Closing DummyCommunication connection");
    isConnected_ = false;
}


}
