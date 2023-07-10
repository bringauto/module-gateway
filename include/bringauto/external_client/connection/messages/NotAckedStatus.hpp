#pragma once

#include <ExternalProtocol.pb.h>
#include <boost/asio.hpp>

#include <utility>



namespace bringauto::external_client::connection::messages {

class NotAckedStatus {
public:
	NotAckedStatus(const ExternalProtocol::Status &status, boost::asio::io_context &timerContext,
				   std::atomic<bool> &responseHandled, std::mutex &responseHandledMutex): timer_(timerContext),
																						  status_(status),
																						  responseHandled_ {
																								  responseHandled },
																						  responseHandledMutex_ {
																								  responseHandledMutex } {}

	void startTimer(const std::function<void(bool)> &endConnectionFunc);

	void cancelTimer();

	ExternalProtocol::Status getStatus() { return status_; }

	InternalProtocol::Device getDevice();

private:
	void timeoutHandler(const std::function<void(bool)> &endConnectionFunc);

	ExternalProtocol::Status status_ {};

	boost::asio::deadline_timer timer_;

	std::atomic<bool> &responseHandled_;

	std::mutex &responseHandledMutex_;

	static constexpr int statusResponseTimeout_ = 30;
};

}