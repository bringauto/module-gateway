#include <bringauto/external_client/connection/messages/NotAckedStatus.hpp>

#include <bringauto/logging/Logger.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <thread>



namespace bringauto::external_client::connection::messages {

void NotAckedStatus::startTimer(const std::function<void()> &endConnectionFunc) {
	timer_.expires_from_now(boost::posix_time::seconds(statusResponseTimeout_));

	timer_.async_wait([this, endConnectionFunc](const boost::system::error_code &errorCode) {
		if(errorCode != boost::asio::error::operation_aborted) {
			timeoutHandler(endConnectionFunc);
		}
	});
}

void NotAckedStatus::cancelTimer() {
	timer_.cancel();
}

void NotAckedStatus::timeoutHandler(const std::function<void()> &endConnectionFunc) {
	std::string loggingStr("Status response Timeout (" + std::to_string(status_.messagecounter()) + "):");
	std::unique_lock <std::mutex> lock(responseHandledMutex_);
	if(responseHandled_.load()) {
		logging::Logger::logError("{} already handled, skipping.", loggingStr);
		return;
	}
	responseHandled_.store(true);    // Is changed back to false in endConnection -> cancelAllTimers
	logging::Logger::logError("{} putting reconnect event onto queue.", loggingStr);

	endConnectionFunc();
}

const ExternalProtocol::Status &NotAckedStatus::getStatus() { return status_; }

const InternalProtocol::Device &NotAckedStatus::getDevice() {
	return status_.devicestatus().device();
}

}