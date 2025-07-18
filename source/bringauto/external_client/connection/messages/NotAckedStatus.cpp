#include <bringauto/external_client/connection/messages/NotAckedStatus.hpp>
#include <bringauto/settings/LoggerId.hpp>
#include <bringauto/settings/Constants.hpp>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>



namespace bringauto::external_client::connection::messages {

void NotAckedStatus::startTimer(const std::function<void()> &endConnectionFunc) {
	timer_.expires_from_now(boost::posix_time::seconds(settings::status_response_timeout.count()));

	timer_.async_wait([this, endConnectionFunc](const boost::system::error_code &errorCode) {
		if(errorCode != boost::asio::error::operation_aborted) {
			timeoutHandler(endConnectionFunc);
		}
	});
}

void NotAckedStatus::cancelTimer() {
	timer_.cancel();
}

void NotAckedStatus::timeoutHandler(const std::function<void()> &endConnectionFunc) const {
	std::string loggingStr("Status response Timeout (" + std::to_string(status_.messagecounter()) + "):");
	std::unique_lock<std::mutex> lock(responseHandledMutex_);
	if(responseHandled_.load()) {
		settings::Logger::logError("{} already handled, skipping.", loggingStr);
		return;
	}
	responseHandled_.store(true);    // Is changed back to false in endConnection -> cancelAllTimers
	settings::Logger::logError("{} putting reconnect event onto queue.", loggingStr);

	endConnectionFunc();
}

const ExternalProtocol::Status &NotAckedStatus::getStatus() const { return status_; }

const InternalProtocol::Device &NotAckedStatus::getDevice() const {
	return status_.devicestatus().device();
}

}
