#pragma once

#include <cstdint>
#include <queue>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace core {

template<class Data, class Compare = std::less<Data> >
class ConcurrentQueue {
private:
	std::priority_queue<Data, std::vector<Data>, Compare> _queue;
	mutable std::mutex _mutex;
	std::condition_variable _conditionVariable;
	std::atomic_bool _abort { false };
public:
	~ConcurrentQueue() {
		abortWait();
	}

	void abortWait() {
		_abort = true;
		_conditionVariable.notify_all();
	}

	void push(Data const& data) {
		std::unique_lock<std::mutex> lock(_mutex);
		_queue.push(data);
		lock.unlock();
		_conditionVariable.notify_one();
	}

	void push(Data&& data) {
		std::unique_lock<std::mutex> lock(_mutex);
		_queue.push(data);
		lock.unlock();
		_conditionVariable.notify_one();
	}

	inline bool empty() const {
		std::unique_lock<std::mutex> lock(_mutex);
		return _queue.empty();
	}

	inline uint32_t size() const {
		std::unique_lock<std::mutex> lock(_mutex);
		return _queue.size();
	}

	bool pop(Data& poppedValue) {
		std::unique_lock<std::mutex> lock(_mutex);
		if (_queue.empty()) {
			return false;
		}

		poppedValue = std::move(_queue.top());
		_queue.pop();
		return true;
	}

	bool waitAndPop(Data& poppedValue) {
		std::unique_lock<std::mutex> lock(_mutex);
		while (_queue.empty()) {
			_conditionVariable.wait(lock, [this] {
				return _abort || !_queue.empty();
			});
			if (_abort) {
				_abort = false;
				return false;
			}
		}

		poppedValue = std::move(_queue.top());
		_queue.pop();
		return true;
	}
};

}
