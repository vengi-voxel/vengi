/**
 * @file
 */

#pragma once

#include "core/concurrent/Atomic.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ConditionVariable.h"
#include "core/Trace.h"
#include "core/Common.h"
#include <stdint.h>
#include <vector>
#include <algorithm>

namespace core {

template<class Data, class Comparator = Less<Data>>
class ConcurrentPriorityQueue {
private:
	using Collection = std::vector<Data>;
	Collection _data;
	mutable core_trace_mutex(core::Lock,  _mutex, "ConcurrentPriorityQueue");
	core::ConditionVariable _conditionVariable;
	core::AtomicBool _abort { false };
	Comparator _comparator;
public:
	using value_type = Data;
	using Key = Data;

	ConcurrentPriorityQueue(size_t reserve = 0u) :
			_comparator(Comparator()) {
		if (reserve) {
			_data.reserve(reserve);
		}
	}
	ConcurrentPriorityQueue(Comparator comparator, size_t reserve = 0u) :
			_comparator(comparator) {
		if (reserve) {
			_data.reserve(reserve);
		}
	}

	~ConcurrentPriorityQueue() {
		abortWait();
	}

	void setComparator(Comparator comparator) {
		core::ScopedLock lock(_mutex);
		_comparator = comparator;
		if (_data.empty()) {
			return;
		}
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
		core::ScopedLock lock(_mutex);
		_data.clear();
	}

	void release() {
		core::ScopedLock lock(_mutex);
		_data.release();
	}

	void sort() {
		core::ScopedLock lock(_mutex);
		if (_data.empty()) {
			return;
		}
		std::make_heap(const_cast<Data*>(&_data.front()), const_cast<Data*>(&_data.front()) + _data.size(), _comparator);
	}

	void push(Data const& data) {
		{
			core::ScopedLock lock(_mutex);
			_data.push_back(data);
			std::push_heap(_data.begin(), _data.end(), _comparator);
		}
		_conditionVariable.notify_one();
	}

	void push(Data&& data) {
		{
			core::ScopedLock lock(_mutex);
			_data.push_back(core::move(data));
			std::push_heap(_data.begin(), _data.end(), _comparator);
		}
		_conditionVariable.notify_one();
	}

	template<typename ... _Args>
	void emplace(_Args&&... __args) {
		{
			core::ScopedLock lock(_mutex);
			_data.emplace_back(core::forward<_Args>(__args)...);
			std::push_heap(_data.begin(), _data.end(), _comparator);
		}
		_conditionVariable.notify_one();
	}

	inline bool empty() const {
		core::ScopedLock lock(_mutex);
		return _data.empty();
	}

	inline uint32_t size() const {
		core::ScopedLock lock(_mutex);
		return (uint32_t)_data.size();
	}

	bool pop(Data& poppedValue) {
		core::ScopedLock lock(_mutex);
		if (_data.empty()) {
			return false;
		}

		poppedValue = core::move(_data.front());
		std::pop_heap(_data.begin(), _data.end(), _comparator);
		_data.pop_back();
		return true;
	}

	bool waitAndPop(Data& poppedValue) {
		core::ScopedLock lock(_mutex);
		while (_data.empty() && !_abort) {
			if (!_conditionVariable.wait(_mutex)) {
				return false;
			}
		}
		if (_abort) {
			return false;
		}
		poppedValue = core::move(_data.front());
		std::pop_heap(_data.begin(), _data.end(), _comparator);
		_data.pop_back();
		return true;
	}
};

}
