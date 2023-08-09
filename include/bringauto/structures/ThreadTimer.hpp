#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <device_management.h>
#include <memory_management.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind/bind.hpp>

#include <chrono>
#include <functional>



namespace bringauto::structures {

/**
 *  Create asynchronous timer which execute specified
 *  function in set time interval
 */
class ThreadTimer {

public:
    /**
     * @brief Constructor
     *
     * @param context   Global context
     * @param func		Function which should be executed
     * @param interval	Interval of time in which function will be executed
     *					(in seconds)
     */
	explicit ThreadTimer(std::shared_ptr <bringauto::structures::GlobalContext> &context,
						 std::function<int(const struct ::device_identification)> fun,
						 const device_identification &deviceId, const long &interval): timer_ { context->ioContext },
																					  fun_ { fun },
																					  deviceId_ { deviceId },
																					  interval_ { interval } {}

	~ThreadTimer();

	void tick(const boost::system::error_code &errorCode);

	/**
	 * Starting the timer
	 */
	void start();

	/*
	 *  Stopping the timer
	 */
	void stop();

	/*
	 *  Restarts the timer
	 */
	void restart();

private:

	boost::asio::deadline_timer timer_;

	std::function<int(const struct ::device_identification)> fun_;

	device_identification deviceId_;

	boost::posix_time::seconds interval_;
};

}