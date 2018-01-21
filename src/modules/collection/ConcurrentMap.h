/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <map>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace core {

template<class Key, class Data, typename Compare = std::less<Key>>
class ConcurrentMap {
private:
	using Collection = std::map<Key, Data, Compare>;
	Collection _map;
	mutable std::mutex _mutex;
	std::condition_variable _conditionVariable;
	std::atomic_bool _abort { false };
public:
	~ConcurrentMap() {
		abortWait();
	}

	void abortWait() {
		_abort = true;
		_conditionVariable.notify_all();
	}

	void clear() {
		std::unique_lock<std::mutex> lock(_mutex);
		_map.clear();
	}

	bool insert(const Key& key, const Data& data) {
		std::unique_lock<std::mutex> lock(_mutex);
		const bool inserted = _map.insert(key, data).second;
		lock.unlock();
		_conditionVariable.notify_one();
		return inserted;
	}

	inline bool empty() const {
		std::unique_lock<std::mutex> lock(_mutex);
		return _map.empty();
	}

	inline uint32_t size() const {
		std::unique_lock<std::mutex> lock(_mutex);
		return _map.size();
	}

	bool pop(Data& poppedValue) {
		std::unique_lock<std::mutex> lock(_mutex);
		if (_map.empty()) {
			return false;
		}

		poppedValue = std::move(_map.first());
		_map.erase(_map.first());
		return true;
	}

	bool waitAndPop(Data& poppedValue) {
		std::unique_lock<std::mutex> lock(_mutex);
		while (_map.empty()) {
			_conditionVariable.wait(lock, [this] {
				return _abort || !_map.empty();
			});
			if (_abort) {
				_abort = false;
				return false;
			}
		}

		poppedValue = std::move(_map.first());
		_map.erase(_map.first());
		return true;
	}
};

}
