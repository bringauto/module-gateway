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
	/**
	 * @brief Handler which is called when timer expires
	 *
	 * @param endConnectionFunc function which is called when status does not get response
	 */
	void timeoutHandler(const std::function<void()> &endConnectionFunc);
	/// Status message that was not acknowledged yet
	ExternalProtocol::Status status_ {};
	/// Timer for checking if status got response
	boost::asio::deadline_timer timer_;
	/// Flag which indicates if status got response
	std::atomic<bool> &responseHandled_;
	std::mutex &responseHandledMutex_;
};

}
