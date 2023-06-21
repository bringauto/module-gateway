#include <bringauto/external_client/connection/messages/NotAckedStatus.hpp>

#include <bringauto/logging/Logger.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <thread>


namespace bringauto::external_client::connection::messages {

NotAckedStatus::NotAckedStatus(ExternalProtocol::Status status, boost::asio::io_context& timerContext) : timer_(timerContext), status_(std::move(status)) {
}

void NotAckedStatus::startTimer(std::atomic<bool> &responseHandled, std::mutex &responseHandledMutex) {
	responseHandled_ = &responseHandled;
	responseHandledMutex_ = &responseHandledMutex;
	timer_.expires_from_now(boost::posix_time::seconds(statusResponseTimeout_));

	timer_.async_wait([this](const boost::system::error_code& error) {
		if (!error) {
			timeoutHandler();
		}
	});
}

void NotAckedStatus::cancelTimer() {
	timer_.cancel();
}

void NotAckedStatus::timeoutHandler() {
	std::string loggingStr("Status response Timeout (" +  std::to_string(status_.messagecounter()) + "):");
	std::unique_lock<std::mutex> lock(*responseHandledMutex_);
	if (responseHandled_) {
		logging::Logger::logError("{} already handled, skipping.", loggingStr);
		return;
	}
	responseHandled_->exchange(true);
	logging::Logger::logError("{} putting event onto queue.", loggingStr);
	// TODO event queue?
}

InternalProtocol::Device NotAckedStatus::getDevice() {
	return status_.devicestatus().device();
}

}