#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace core {

template<class Data>
class ConcurrentVector {
private:
	using Collection = std::vector<Data>;
	Collection _data;
	mutable std::mutex _mutex;
	std::condition_variable _conditionVariable;
	std::atomic_bool _abort { false };
public:
	~ConcurrentVector() {
		abortWait();
	}

	void abortWait() {
		_abort = true;
		_conditionVariable.notify_all();
	}

	void clear() {
		std::unique_lock<std::mutex> lock(_mutex);
		_data.clear();
	}

	void push(Data const& data) {
		std::unique_lock<std::mutex> lock(_mutex);
		_data.push_back(data);
		lock.unlock();
		_conditionVariable.notify_one();
	}

	std::vector<Data> clearCopy() {
		std::unique_lock<std::mutex> lock(_mutex);
		std::vector<Data> copy = _data;
		_data.clear();
		return copy;
	}

	void push(Data&& data) {
		std::unique_lock<std::mutex> lock(_mutex);
		_data.emplace_back(std::forward(data));
		lock.unlock();
		_conditionVariable.notify_one();
	}

	inline bool empty() const {
		std::unique_lock<std::mutex> lock(_mutex);
		return _data.empty();
	}

	inline uint32_t size() const {
		std::unique_lock<std::mutex> lock(_mutex);
		return _data.size();
	}

	bool pop(Data& poppedValue) {
		std::unique_lock<std::mutex> lock(_mutex);
		if (_data.empty()) {
			return false;
		}

		poppedValue = std::move(_data.back());
		_data.pop_back();
		return true;
	}

	bool waitAndPop(Data& poppedValue) {
		std::unique_lock<std::mutex> lock(_mutex);
		while (_data.empty()) {
			_conditionVariable.wait(lock, [this] {
				return _abort || !_data.empty();
			});
			if (_abort) {
				_abort = false;
				return false;
			}
		}

		poppedValue = std::move(_data.back());
		_data.pop_back();
		return true;
	}

	template<class SORT>
	void sort(SORT&& sort, int amount = 0) {
		std::unique_lock<std::mutex> lock(_mutex);
		if (amount <= 0) {
			std::sort(_data.begin(), _data.end(), sort);
		} else {
			std::partial_sort(_data.begin(), _data.begin() + amount, _data.end(), sort);
		}
	}
};

}
