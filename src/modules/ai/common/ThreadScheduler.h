#pragma once

#include <chrono>
#include <vector>
#include <functional>
#include <queue>
#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace ai {

/**
 * Schedules tasks that executed with a fixed delay between their executions.
 */
class ThreadScheduler {
private:
	struct ScheduledTask {
		ThreadScheduler* _scheduler;
		std::function<void()> _callback;
		std::chrono::milliseconds _delay;
		std::chrono::milliseconds _execTime;

		ScheduledTask(ThreadScheduler* scheduler, std::function<void()>&& callback, const std::chrono::milliseconds& initialDelay, const std::chrono::milliseconds& delay) :
				_scheduler(scheduler), _callback(callback), _delay(delay), _execTime(initialDelay) {
		}

		ScheduledTask(ThreadScheduler* scheduler, const std::function<void()>& callback, const std::chrono::milliseconds& initialDelay, const std::chrono::milliseconds& delay) :
				_scheduler(scheduler), _callback(callback), _delay(delay), _execTime(initialDelay) {
		}

		void operator()() const {
			_callback();
			if (_delay.count() <= 0) {
				return;
			}
			// reschedule with the same delay from that moment the callback finished
			auto epoch = std::chrono::system_clock::now().time_since_epoch();
			auto now = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
			_scheduler->_tasks.emplace(_scheduler, _callback, now + _delay, _delay);
		}

		inline bool operator<(const ScheduledTask& other) const {
			return other._execTime < _execTime;
		}
	};

	std::priority_queue<ScheduledTask, std::vector<ScheduledTask> > _tasks;
	std::atomic_bool _stop;
	std::atomic_bool _notEmpty;
	std::mutex _queueMutex;
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
					std::unique_lock<std::mutex> lock(_queueMutex);
					while (!this->_tasks.empty() && this->_tasks.top()._execTime <= now) {
						if (this->_stop) {
							return;
						}
						//std::async(std::launch::async, this->_tasks.top());
						this->_tasks.top()();
						this->_tasks.pop();
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

	template<class F, class ... Args>
	void schedule(const std::chrono::milliseconds& delay, F&& f, Args&&... args) {
		scheduleAtFixedRate(delay, std::chrono::milliseconds(), std::forward<F>(f), std::forward<Args>(args)...);
	}

	/**
	 * @param[in] f The task to execute
	 * @param[in] initialDelay The milliseconds to delay the first execution
	 * @param[in] period The period in milliseconds between successive executions
	 */
	template<class F, class ... Args>
	void scheduleAtFixedRate(const std::chrono::milliseconds& initialDelay, const std::chrono::milliseconds& delay, F&& f, Args&&... args) {
		auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		auto epoch = std::chrono::system_clock::now().time_since_epoch();
		auto now = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
		std::unique_lock<std::mutex> lock(_queueMutex);
		_tasks.emplace(this, std::move<std::function<void()> >(task), now + initialDelay, delay);
		_notEmpty = true;
	}
};
}
