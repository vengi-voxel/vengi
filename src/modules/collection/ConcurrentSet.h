#pragma once

#include <cstdint>
#include <set>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <algorithm>

namespace core {

template<class Data, class Compare = std::less<Data> >
class ConcurrentSet {
private:
	using Collection = std::set<Data, Compare>;
	Collection _data;
	mutable std::mutex _mutex;
	std::condition_variable _conditionVariable;
	std::atomic_bool _abort { false };
public:
	~ConcurrentSet() {
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

	bool push(Data const& data) {
		std::unique_lock<std::mutex> lock(_mutex);
		auto i = _data.insert(data);
		lock.unlock();
		_conditionVariable.notify_one();
		return i->second;
	}

	bool contains(Data const& data) const {
		std::unique_lock<std::mutex> lock(_mutex);
		return _data.find(data) != _data.end();
	}

	bool push(Data&& data) {
		std::unique_lock<std::mutex> lock(_mutex);
		auto i = _data.insert(data);
		lock.unlock();
		_conditionVariable.notify_one();
		return i->second;
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

		poppedValue = std::move(*_data.begin());
		_data.erase(_data.begin());
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

		poppedValue = std::move(*_data.begin());
		_data.erase(_data.begin());
		return true;
	}

	template<class SORT>
	void sort(SORT&& sort) {
		std::unique_lock<std::mutex> lock(_mutex);
		std::sort(_data.begin(), _data.end(), sort);
	}
};

}
