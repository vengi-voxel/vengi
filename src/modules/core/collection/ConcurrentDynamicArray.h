/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Lock.h"
#include <stdint.h>

namespace core {

template<class Data, size_t INCREASE = 32u>
class ConcurrentDynamicArray {
private:
	using Collection = core::DynamicArray<Data, INCREASE>;
	Collection _data core_thread_guarded_by(_mutex);
	mutable core_trace_mutex(core::Lock, _mutex, "ConcurrentDynamicArray");

public:
	using value_type = Data;

	ConcurrentDynamicArray(size_t reserve = 0u) {
		if (reserve) {
			_data.reserve(reserve);
		}
	}

	size_t size() const {
		core::ScopedLock lock(_mutex);
		return _data.size();
	}

	void clear() {
		core::ScopedLock lock(_mutex);
		_data.clear();
	}

	void release() {
		core::ScopedLock lock(_mutex);
		_data.release();
	}

	void push(Data const &data) {
		core::ScopedLock lock(_mutex);
		_data.push_back(data);
	}

	void push(Data &&data) {
		core::ScopedLock lock(_mutex);
		_data.push_back(core::move(data));
	}

	template<typename ITER>
	void replace(ITER first, ITER last) {
		core::ScopedLock lock(_mutex);
		_data.clear();
		_data.insert(_data.begin(), first, last);
	}

	template<typename ITER>
	void append(ITER first, ITER last) {
		core::ScopedLock lock(_mutex);
		_data.insert(_data.end(), first, last);
	}

	template <typename... _Args>
	void emplace_back(_Args &&... __args) {
		core::ScopedLock lock(_mutex);
		_data.emplace_back(core::forward<_Args>(__args)...);
	}

	inline bool empty() const {
		core::ScopedLock lock(_mutex);
		return _data.empty();
	}

	bool pop(Data &poppedValue) {
		core::ScopedLock lock(_mutex);
		if (_data.empty()) {
			return false;
		}

		poppedValue = core::move(_data.back());
		_data.pop();
		return true;
	}

	bool get(size_t index, Data &target) {
		core::ScopedLock lock(_mutex);
		if (index >= _data.size()) {
			return false;
		}

		target = _data[index];
		return true;
	}
};

}
