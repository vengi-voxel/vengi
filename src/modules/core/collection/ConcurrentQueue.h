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

template<class T>
struct Less {
	constexpr bool operator()(const T &lhs, const T &rhs) const {
		return lhs < rhs;
	}
};

template<class Data, class Comparator = Less<Data>>
class ConcurrentQueue {
private:
	using Collection = std::vector<Data>;
	Collection _data;
	mutable core::Lock _mutex;
	core::ConditionVariable _conditionVariable;
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
		core::ScopedLock lock(_mutex);
		_comparator = comparator;
		std::make_heap(const_cast<Data*>(&_data.front()), const_cast<Data*>(&_data.front()) + _data.size(), _comparator);
	}

	void abortWait() {
		_abort = true;
		_conditionVariable.signalAll();
	}

	void reset() {
		_abort = false;
	}

	void clear() {
		core::ScopedLock lock(_mutex);
		_data = Collection();
	}

	void sort() {
		core::ScopedLock lock(_mutex);
		std::make_heap(const_cast<Data*>(&_data.front()), const_cast<Data*>(&_data.front()) + _data.size(), _comparator);
	}

	void push(Data const& data) {
		core::ScopedLock lock(_mutex);
		_data.push_back(data);
		std::push_heap(_data.begin(), _data.end(), _comparator);
		_conditionVariable.signalOne();
	}

	void push(Data&& data) {
		core::ScopedLock lock(_mutex);
		_data.push_back(core::move(data));
		std::push_heap(_data.begin(), _data.end(), _comparator);
		_conditionVariable.signalOne();
	}

	template<typename ... _Args>
	void emplace(_Args&&... __args) {
		core::ScopedLock lock(_mutex);
		_data.emplace_back(core::forward<_Args>(__args)...);
		std::push_heap(_data.begin(), _data.end(), _comparator);
		_conditionVariable.signalOne();
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
		if (_data.empty()) {
			_conditionVariable.wait(_mutex);
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
