#include <bringauto/structures/ThreadTimer.hpp>

#include <bringauto/logging/Logger.hpp>

#include <iostream>



namespace bringauto::structures {

ThreadTimer::~ThreadTimer() {
	stop();
}

void ThreadTimer::tick(const boost::system::error_code &errorCode) {
	if(errorCode == boost::asio::error::operation_aborted || end_.load()) {
		return;
	}
	fun_(deviceId_);
	const std::string& name = deviceId_.getDeviceName();
	logging::Logger::logDebug("Timer expired and force aggregation was invoked on device: {}, {}", name, errorCode.value());
	timer_.expires_from_now(interval_);
	timer_.async_wait([this](const boost::system::error_code &errorCode) {
		tick(errorCode);
	});
}

void ThreadTimer::start() {
	end_.store(false);
	timer_.expires_from_now(interval_);
	timer_.async_wait([this](const boost::system::error_code &errorCode) {
		tick(errorCode);
	});
}

void ThreadTimer::stop() {
	try {
		end_.store(true);
		timer_.cancel();
	} catch(boost::system::system_error &e) {
		std::cerr << "System error in thread timer\n";
	}
}

void ThreadTimer::restart() {
	stop();
	start();
}

}
