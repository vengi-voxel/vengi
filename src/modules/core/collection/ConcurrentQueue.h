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

namespace core {

template<class Data>
class ConcurrentQueue {
private:
	using Collection = std::vector<Data>;
	Collection _data;
	mutable core_trace_mutex(core::Lock,  _mutex, "ConcurrentQueue");
	core::ConditionVariable _conditionVariable;
	core::AtomicBool _abort { false };
public:
	using value_type = Data;
	using Key = Data;

	~ConcurrentQueue() {
		abortWait();
	}

	void abortWait() {
		_abort = true;
		core::ScopedLock lock(_mutex);
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

	void push(Data const& data) {
		core::ScopedLock lock(_mutex);
		_data.push_back(data);
		_conditionVariable.notify_one();
	}

	void push(Data&& data) {
		core::ScopedLock lock(_mutex);
		_data.push_back(core::move(data));
		_conditionVariable.notify_one();
	}

	template<typename ... _Args>
	void emplace(_Args&&... __args) {
		core::ScopedLock lock(_mutex);
		_data.emplace_back(core::forward<_Args>(__args)...);
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
		_data.erase(_data.begin());
		return true;
	}

	bool waitAndPop(Data& poppedValue, uint32_t timeoutMillis = 0u) {
		core::ScopedLock lock(_mutex);
		if (_data.empty()) {
			if (_abort) {
				return false;
			}
			if (timeoutMillis == 0u) {
				if (!_conditionVariable.wait(_mutex)) {
					return false;
				}
			} else {
				const core::ConditionVariableState state = _conditionVariable.waitTimeout(_mutex, timeoutMillis);
				if (state != core::ConditionVariableState::Signaled) {
					return false;
				}
			}
			if (_abort) {
				return false;
			}
		}
		poppedValue = core::move(_data.front());
		_data.erase(_data.begin());
		return true;
	}
};

}
