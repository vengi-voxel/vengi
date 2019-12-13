#pragma once

#include <stdint.h>
#include <queue>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <functional>
#include "core/Trace.h"

namespace core {

template<class Data, class Compare = std::less<Data> >
class ConcurrentQueue {
private:
	using Collection = std::priority_queue<Data, std::vector<Data>, Compare>;
	Collection _data;
	mutable core_trace_mutex(std::mutex, _mutex);
	std::condition_variable_any _conditionVariable;
	std::atomic_bool _abort { false };
public:
	using Key = Data;

	~ConcurrentQueue() {
		abortWait();
	}

	void abortWait() {
		_abort = true;
		_conditionVariable.notify_all();
	}

	void reset() {
		_abort = false;
	}

	void clear() {
		std::unique_lock lock(_mutex);
		_data = Collection();
	}

	void sort(std::function<bool(const Data& lhs, const Data& rhs)> sorter) {
		std::unique_lock lock(_mutex);
		std::make_heap(const_cast<Data*>(&_data.top()), const_cast<Data*>(&_data.top()) + _data.size(), sorter);
	}

	void push(Data const& data) {
		{
			std::unique_lock lock(_mutex);
			_data.push(data);
		}
		_conditionVariable.notify_one();
	}

	void push(Data&& data) {
		{
			std::unique_lock lock(_mutex);
			_data.push(data);
		}
		_conditionVariable.notify_one();
	}

	template<typename ... _Args>
	void emplace(_Args&&... __args) {
		{
			std::unique_lock lock(_mutex);
			_data.emplace(std::forward<_Args>(__args)...);
		}
		_conditionVariable.notify_one();
	}

	inline bool empty() const {
		std::unique_lock lock(_mutex);
		return _data.empty();
	}

	inline uint32_t size() const {
		std::unique_lock lock(_mutex);
		return (uint32_t)_data.size();
	}

	bool pop(Data& poppedValue) {
		std::unique_lock lock(_mutex);
		if (_data.empty()) {
			return false;
		}

		poppedValue = std::move(_data.top());
		_data.pop();
		return true;
	}

	bool waitAndPop(Data& poppedValue) {
		std::unique_lock lock(_mutex);
		_conditionVariable.wait(lock, [this] {
			return _abort || !_data.empty();
		});
		if (_abort) {
			return false;
		}
		poppedValue = std::move(_data.top());
		_data.pop();
		return true;
	}
};

}
