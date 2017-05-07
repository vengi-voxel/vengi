/**
 * @file
 */
#pragma once

#include <chrono>
#include <queue>
#include <functional>
#include <thread>
#include <future>
#include <atomic>
#include <mutex>

namespace ai {

/**
 * Schedules tasks that are executed with a fixed delay between their executions and/or
 * with a delay before their initial execution.
 *
 * @note There is a single thread that checks for scheduled tasks
 */
class ThreadScheduler {
private:
	struct ScheduledTask {
		ThreadScheduler* _scheduler;
		std::function<void()> _callback;
		// if this is > 0 the task is rescheduled
		std::chrono::milliseconds _delay;
		std::chrono::milliseconds _execTime;
		int _timerId;

		ScheduledTask(ThreadScheduler* scheduler, std::function<void()>&& callback, const std::chrono::milliseconds& initialDelay, const std::chrono::milliseconds& delay, int timerId) :
				_scheduler(scheduler), _callback(callback), _delay(delay), _execTime(initialDelay), _timerId(timerId) {
		}

		/**
		 * @return @c true if the task should get rescheduled
		 */
		bool operator()() const {
			_callback();
			return _delay.count() > 0;
		}

		inline bool operator<(const ScheduledTask& other) const {
			return other._execTime < _execTime;
		}
	};

	class prioqueue: public std::priority_queue<ScheduledTask> {
	public:
		bool remove(int timerId) {
			auto it = std::find_if(this->c.begin(), this->c.end(), [=] (const ScheduledTask& task) { return task._timerId == timerId; });
			if (it == this->c.end()) {
				return false;
			};
			this->c.erase(it);
			return true;
		}
	};
	prioqueue _tasks;
	std::atomic_int _timerId {0};
	std::atomic_bool _stop;
	std::atomic_bool _notEmpty;
	std::mutex _mutex;
	std::thread _thread;

public:
	ThreadScheduler() : _stop(false), _notEmpty(false) {
		_thread = std::thread([this] {
			for (;;) {
				if (this->_stop) {
					return;
				}
				if (this->_notEmpty) {
					auto epoch = std::chrono::system_clock::now().time_since_epoch();
					auto now = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
					std::unique_lock<std::mutex> lock(_mutex);
					while (!this->_tasks.empty() && this->_tasks.top()._execTime <= now) {
						if (this->_stop) {
							return;
						}
						const ScheduledTask& t = this->_tasks.top();
						if (t()) {
							auto callback = t._callback;
							auto delay = t._delay;
							auto timerId = t._timerId;
							auto execTime = t._execTime;
							this->_tasks.pop();
							_tasks.emplace(this, std::move(callback), execTime + delay, delay, timerId);
						} else {
							this->_tasks.pop();
						}
					}
					if (this->_stop) {
						return;
					}
					this->_notEmpty = !this->_tasks.empty();
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		});
	}

	/**
	 * Will not wait for scheduled tasks
	 */
	~ThreadScheduler() {
		_stop = true;
		_thread.join();
	}

	/**
	 * @brief Executes the given functor once after the given delay has passed.
	 * @param[in] f The task to execute
	 * @param[in] delay The period in milliseconds that must have been passed for the execution
	 * @return The timer id
	 */
	template<class F, class ... Args>
	inline int schedule(const std::chrono::milliseconds& delay, F&& f, Args&&... args) {
		return scheduleAtFixedRate(delay, std::chrono::milliseconds(), std::forward<F>(f), std::forward<Args>(args)...);
	}

	/**
	 * @brief Schedules a task for continuous execution
	 * @param[in] f The task to execute
	 * @param[in] initialDelay The milliseconds to delay the first execution
	 * @param[in] delay The period in milliseconds between successive executions
	 * @return The timer id
	 */
	template<class F, class ... Args>
	int scheduleAtFixedRate(const std::chrono::milliseconds& initialDelay, const std::chrono::milliseconds& delay, F&& f, Args&&... args) {
		auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		auto epoch = std::chrono::system_clock::now().time_since_epoch();
		auto now = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
		const int timerId = ++_timerId;
		std::unique_lock<std::mutex> lock(_mutex);
		_tasks.emplace(this, std::move(task), now + initialDelay, delay, timerId);
		_notEmpty = true;
		return timerId;
	}

	/**
	 * @brief Cancels the given timer
	 * @param[in] timerId The timer id returned by schedule() or scheduleAtFixedRate()
	 * @return @c true if the timer was canceled, @c false otherwise
	 */
	bool cancel(int timerId) {
		std::unique_lock<std::mutex> lock(_mutex);
		return _tasks.remove(timerId);
	}
};

}
