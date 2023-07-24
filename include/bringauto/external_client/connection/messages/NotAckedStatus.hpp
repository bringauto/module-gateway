#pragma once

#include <ExternalProtocol.pb.h>
#include <boost/asio.hpp>

#include <utility>



namespace bringauto::external_client::connection::messages {

class NotAckedStatus {
public:
	NotAckedStatus(const ExternalProtocol::Status &status, boost::asio::io_context &timerContext,
				   std::atomic<bool> &responseHandled, std::mutex &responseHandledMutex): status_{status},
																						  timer_{timerContext},
																						  responseHandled_ {
																								  responseHandled },
																						  responseHandledMutex_ {
																								  responseHandledMutex } {}

	void startTimer(const std::function<void()> &endConnectionFunc);

	void cancelTimer();

	const ExternalProtocol::Status &getStatus();

	const InternalProtocol::Device &getDevice();

private:
	void timeoutHandler(const std::function<void()> &endConnectionFunc);

	ExternalProtocol::Status status_;

	boost::asio::deadline_timer timer_;

	std::atomic<bool> &responseHandled_;

	std::mutex &responseHandledMutex_;

	static constexpr int statusResponseTimeout_ = 30;
};

}