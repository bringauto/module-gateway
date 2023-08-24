#pragma once

#include <ExternalProtocol.pb.h>
#include <boost/asio.hpp>

#include <utility>



namespace bringauto::external_client::connection::messages {

/**
 * @brief Class checks if status got status response message in time
 */
class NotAckedStatus {
public:
	NotAckedStatus(const ExternalProtocol::Status &status, boost::asio::io_context &timerContext,
				   std::atomic<bool> &responseHandled, std::mutex &responseHandledMutex): status_ { status },
																						  timer_ { timerContext },
																						  responseHandled_ {
																								  responseHandled },
																						  responseHandledMutex_ {
																								  responseHandledMutex } {}

	/**
	 * @brief Start timer
	 *
	 * @param endConnectionFunc function which is called when status does not get response
	 */
	void startTimer(const std::function<void()> &endConnectionFunc);

	/**
	 * @brief Cancel timer
	 */
	void cancelTimer();

	/**
	 * @brief Get status message
	 *
	 * @return const ExternalProtocol::Status&
	 */
	const ExternalProtocol::Status &getStatus() const;

	/**
	 * @brief Get device
	 *
	 * @return const InternalProtocol::Device&
	 */
	const InternalProtocol::Device &getDevice() const;

private:
	void timeoutHandler(const std::function<void()> &endConnectionFunc);

	ExternalProtocol::Status status_;

	boost::asio::deadline_timer timer_;

	std::atomic<bool> &responseHandled_;

	std::mutex &responseHandledMutex_;
};

}