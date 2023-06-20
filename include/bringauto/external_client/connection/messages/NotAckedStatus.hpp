#pragma once

#include <ExternalProtocol.pb.h>
#include <boost/asio.hpp>

#include <utility>

namespace bringauto::external_client::connection::messages {

class NotAckedStatus {
public:
	NotAckedStatus(ExternalProtocol::Status status, boost::asio::io_context& timerContext);
	void startTimer(std::atomic<bool>& responseHandled, std::mutex& responseHandledMutex);
	void cancelTimer();

	ExternalProtocol::Status getStatus() { return status_; }
private:
	void timeoutHandler();

	ExternalProtocol::Status status_ {};

	boost::asio::deadline_timer timer_;

	std::atomic<bool>* responseHandled_;
	std::mutex* responseHandledMutex_;
	std::thread timerThread_;

	static constexpr int statusResponseTimeout_ = 30;
};

}