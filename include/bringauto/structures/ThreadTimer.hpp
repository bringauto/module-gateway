#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/settings/Constants.hpp>
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
	explicit ThreadTimer(std::shared_ptr<bringauto::structures::GlobalContext> &context,
						 std::function<int(const struct ::device_identification)> &function,
						 const device_identification &deviceId): timer_ { context->ioContext },
																 fun_ { function },
																 deviceId_ { deviceId } {}

	~ThreadTimer();

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

	void tick(const boost::system::error_code &errorCode);

	boost::asio::deadline_timer timer_;

	std::atomic_bool end_ { false };

	std::function<int(const struct ::device_identification)> fun_;

	device_identification deviceId_;

	boost::posix_time::seconds interval_ { settings::status_aggregation_timeout.count() };
};

}