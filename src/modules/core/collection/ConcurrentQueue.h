/**
 * @file
 */

#pragma once

#include <stdint.h>
#include "core/Atomic.h"
#include <vector>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <algorithm>
#include "core/Trace.h"

namespace core {

template<class Data, class Comparator = std::less<Data>>
class ConcurrentQueue {
private:
	using Collection = std::vector<Data>;
	Collection _data;
	mutable core_trace_mutex(std::mutex, _mutex);
	std::condition_variable_any _conditionVariable;
	core::AtomicBool _abort { false };
	Comparator _comparator;
public:
	using Key = Data;

	ConcurrentQueue() :
			_comparator(Comparator()) {
	}
	ConcurrentQueue(Comparator comparator) :
			_comparator(comparator) {
	}
	~ConcurrentQueue() {
		abortWait();
	}

	void setComparator(Comparator comparator) {
		std::unique_lock lock(_mutex);
		_comparator = comparator;
		std::make_heap(const_cast<Data*>(&_data.front()), const_cast<Data*>(&_data.front()) + _data.size(), _comparator);
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

	void sort() {
		std::unique_lock lock(_mutex);
		std::make_heap(const_cast<Data*>(&_data.front()), const_cast<Data*>(&_data.front()) + _data.size(), _comparator);
	}

	void push(Data const& data) {
		{
			std::unique_lock lock(_mutex);
			_data.push_back(data);
			std::push_heap(_data.begin(), _data.end(), _comparator);
		}
		_conditionVariable.notify_one();
	}

	void push(Data&& data) {
		{
			std::unique_lock lock(_mutex);
			_data.push_back(std::move(data));
			std::push_heap(_data.begin(), _data.end(), _comparator);
		}
		_conditionVariable.notify_one();
	}

	template<typename ... _Args>
	void emplace(_Args&&... __args) {
		{
			std::unique_lock lock(_mutex);
			_data.emplace_back(std::forward<_Args>(__args)...);
			std::push_heap(_data.begin(), _data.end(), _comparator);
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

		poppedValue = std::move(_data.front());
		std::pop_heap(_data.begin(), _data.end(), _comparator);
		_data.pop_back();
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
		poppedValue = std::move(_data.front());
		std::pop_heap(_data.begin(), _data.end(), _comparator);
		_data.pop_back();
		return true;
	}
};

}
