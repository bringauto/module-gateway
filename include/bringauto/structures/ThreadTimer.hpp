#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>
#include <bringauto/settings/Constants.hpp>

#include <fleet_protocol/common_headers/device_management.h>
#include <fleet_protocol/common_headers/memory_management.h>
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
class ThreadTimer final {

public:
	/**
	 * @brief Constructor
	 *
	 * @param context   Global context
	 * @param func		Function which should be executed
	 * @param interval	Interval of time in which function will be executed
	 *					(in seconds)
	 */
	explicit ThreadTimer(std::shared_ptr<GlobalContext> &context,
						 std::function<int(const DeviceIdentification&)> &function,
						 const DeviceIdentification& deviceId): timer_ { context->ioContext },
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

	/**
	 * @brief Flag which indicates if timer should be stopped
	 */
	std::atomic_bool end_ { false };

	/**
	 * @brief Function which should be executed if timer expires
	 */
	std::function<int(const DeviceIdentification&)> fun_ {};
	/// Device identification struct
	DeviceIdentification deviceId_ {};
	/// Interval of time in which timer time out
	boost::posix_time::seconds interval_ { settings::status_aggregation_timeout.count() };
};

}
