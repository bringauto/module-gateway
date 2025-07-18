#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>



namespace bringauto::structures {


/**
 * Thread safe queue implementation
 * - inspired by std::queue
 * - does not implement all std::queue primitive functions
 * @tparam T class type
 */
template <typename T>
class AtomicQueue {
public:
	/**
	 * @brief Add data to the end of the queue and then notifies waiting thread.
	 * @param value class T object
	 */
	void pushAndNotify(const T &value) {
		std::lock_guard<std::mutex> lock(mtx_);
		queue_.push(value);
		cv_.notify_one();
	}

	/**
	 * @brief Waits for timeout or till being notified that queue is not empty.
	 * @param timeout length of timeout
	 * @return true if the queue is empty
	 */
	bool waitForValueWithTimeout(const std::chrono::seconds &timeout) {
		std::unique_lock<std::mutex> lock(mtx_);
		cv_.wait_for(lock, timeout, [this]() { return !queue_.empty(); });
		return queue_.empty();
	}

	/**
	 * @brief Add data to the end of the queue.
	 * @param value class T object
	 */
	void push(const T &value) {
		std::lock_guard<std::mutex> lock(mtx_);
		queue_.push(value);
	}

	/**
	 * @brief Removes first element in queue.
	 */
	void pop() {
		std::lock_guard<std::mutex> lock(mtx_);
		queue_.pop();
	}

	/**
	 * @brief Checks for state of queue.
	 * @return true if the queue is empty
	 */
	bool empty() {
		std::lock_guard<std::mutex> lock(mtx_);
		return queue_.empty();
	}

	/**
	 * @brief Gets read/write reference to the data at the first element of the queue.
	 * @return reference to the data
	 */
	T &front() {
		std::lock_guard<std::mutex> lock(mtx_);
		return queue_.front();
	}

	/**
	 * @brief Checks for the number of elements in the queue.
	 * @return the number of elements in the queue
	 */
	size_t size() {
		std::lock_guard<std::mutex> lock(mtx_);
		return queue_.size();
	}

private:
	std::queue<T> queue_ {};
	std::mutex mtx_ {};
	std::condition_variable cv_ {};
};

}
