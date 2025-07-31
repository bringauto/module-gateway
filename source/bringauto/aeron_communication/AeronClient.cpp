#include <bringauto/aeron_communication/AeronClient.hpp>
#include <bringauto/settings/Constants.hpp>
#include <bringauto/settings/LoggerId.hpp>



namespace bringauto::aeron_communication {

using log = settings::Logger;

AeronClient::AeronClient() {
	aeronContext_.newPublicationHandler(
		[](const std::string &channel, std::int32_t streamId, std::int32_t sessionId, std::int64_t correlationId) {
			log::logDebug("AeronClient: New publication on channel: " + channel +
						  " with correlationId: " + std::to_string(correlationId) +
						  ", streamId: " + std::to_string(streamId) +
						  ", sessionId: " + std::to_string(sessionId)
			);
		}
	);

	aeronContext_.newSubscriptionHandler(
		[](const std::string &channel, std::int32_t streamId, std::int64_t correlationId) {
			log::logDebug("AeronClient: New subscription on channel: " + channel +
						  " with correlationId: " + std::to_string(correlationId) +
						  ", streamId: " + std::to_string(streamId)
			);
		}
	);

	aeronContext_.availableImageHandler(
		[](aeron::Image &image) {
			log::logDebug("AeronClient: Available image on correlationId: " + std::to_string(image.correlationId()) +
						  ", sessionId: " + std::to_string(image.sessionId()) +
						  ", position: " + std::to_string(image.position()) +
						  ", sourceIdentity: " + image.sourceIdentity()
			);
		}
	);

	aeronContext_.unavailableImageHandler(
		[](aeron::Image &image) {
			log::logDebug("AeronClient: Unavailable image on correlationId: " + std::to_string(image.correlationId()) +
						  ", sessionId: " + std::to_string(image.sessionId()) +
						  ", position: " + std::to_string(image.position()) +
						  ", sourceIdentity: " + image.sourceIdentity()
			);
		}
	);

	aeron_ = aeron::Aeron::connect(aeronContext_);
    aeronFragmentAssembler_ = std::make_shared<aeron::FragmentAssembler>(handleMessage());
    aeronHandler_ = std::make_shared<aeron::fragment_handler_t>(aeronFragmentAssembler_->handler());
    aeronIdleStrategy_ = std::make_shared<aeron::YieldingIdleStrategy>();
}


void AeronClient::addModule(uint16_t moduleId) {
	std::int64_t id = aeron_->addPublication(
		std::string(settings::Constants::AERON_CONNECTION),
		settings::aeron_to_module_stream_id_base + moduleId	
	);
	aeronPublication_ = aeron_->findPublication(id);
	while (!aeronPublication_) {
		std::this_thread::yield();
		aeronPublication_ = aeron_->findPublication(id);
	}

	id = aeron_->addSubscription(
		std::string(settings::Constants::AERON_CONNECTION),
		settings::aeron_to_gateway_stream_id_base + moduleId	
	);
	aeronSubscription_ = aeron_->findSubscription(id);
	while (!aeronSubscription_) {
		std::this_thread::yield();
		aeronSubscription_ = aeron_->findSubscription(id);
	}
}


void AeronClient::callModuleFunction(ModuleFunctions function, const std::string &message) {
	std::string fullMessage = std::string(moduleFunctionToCode(function)) + ":" + message;

	std::array<std::uint8_t, 256> buff;
	aeron::concurrent::AtomicBuffer srcBuffer(&buff[0], buff.size());
	char charMessage[256];
	const int messageLen = ::snprintf(charMessage, sizeof(charMessage), "%s", fullMessage.c_str());
	srcBuffer.putBytes(0, reinterpret_cast<std::uint8_t *>(charMessage), messageLen);
	aeronPublication_->offer(srcBuffer, 0, messageLen);
	waitForAeronResponse();
}


std::string_view AeronClient::getMessage() const {
	return aeronMessage_;
}


bool AeronClient::waitForAeronResponse() {
	aeronPolling_ = true;
	while (aeronPolling_) {
		const int fragmentsRead = aeronSubscription_->poll(*aeronHandler_, 10);
		aeronIdleStrategy_->idle(fragmentsRead);
	}
	return true;
}


aeron::fragment_handler_t AeronClient::handleMessage() {
	return [&](const aeron::AtomicBuffer &buffer, aeron::util::index_t offset, aeron::util::index_t length, const aeron::Header &header) {
		std::string message(reinterpret_cast<const char *>(buffer.buffer()) + offset, static_cast<std::size_t>(length));
		aeronMessage_ = message;
		aeronPolling_ = false;
	};
}

std::string AeronClient::moduleFunctionToCode(ModuleFunctions function) const {
	return std::to_string(static_cast<int>(function));
}

}
