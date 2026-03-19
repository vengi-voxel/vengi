/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ConditionVariable.h"

namespace core {

namespace detail {

template<typename T>
struct FutureState {
	T _value;
	core_trace_mutex(core::Lock, _mutex, "FutureState");
	core::ConditionVariable _cv;
	core::AtomicBool _ready{false};
	core::AtomicInt _refCount{1};

	void addRef() {
		_refCount.increment(1);
	}

	void release() {
		if (_refCount.decrement(1) - 1 == 0) {
			this->~FutureState();
			core_free(this);
		}
	}

	void set(const T &val) {
		core::ScopedLock lock(_mutex);
		_value = val;
		_ready = true;
		_cv.notify_all();
	}

	void set(T &&val) {
		core::ScopedLock lock(_mutex);
		_value = core::move(val);
		_ready = true;
		_cv.notify_all();
	}

	T get() {
		wait();
		return core::move(_value);
	}

	bool ready() const {
		return (bool)_ready;
	}

	void wait() {
		core::ScopedLock lock(_mutex);
		_cv.wait(_mutex, [this]() core_thread_no_thread_safety_analysis { return (bool)_ready; });
	}
};

template<>
struct FutureState<void> {
	core_trace_mutex(core::Lock, _mutex, "FutureStateVoid");
	core::ConditionVariable _cv;
	core::AtomicBool _ready{false};
	core::AtomicInt _refCount{1};

	void addRef() {
		_refCount.increment(1);
	}

	void release() {
		if (_refCount.decrement(1) - 1 == 0) {
			this->~FutureState();
			core_free(this);
		}
	}

	void set() {
		core::ScopedLock lock(_mutex);
		_ready = true;
		_cv.notify_all();
	}

	void get() {
		wait();
	}

	bool ready() const {
		return (bool)_ready;
	}

	void wait() {
		core::ScopedLock lock(_mutex);
		_cv.wait(_mutex, [this]() core_thread_no_thread_safety_analysis { return (bool)_ready; });
	}
};

} // namespace detail

template<typename T>
class Future {
private:
	detail::FutureState<T> *_state = nullptr;

	void releaseState() {
		if (_state != nullptr) {
			_state->release();
			_state = nullptr;
		}
	}

public:
	constexpr Future() {
	}

	Future(detail::FutureState<T> *state) : _state(state) {
	}

	~Future() {
		releaseState();
	}

	Future(const Future &) = delete;
	Future &operator=(const Future &) = delete;

	Future(Future &&other) noexcept : _state(other._state) {
		other._state = nullptr;
	}

	Future &operator=(Future &&other) noexcept {
		if (this != &other) {
			releaseState();
			_state = other._state;
			other._state = nullptr;
		}
		return *this;
	}

	bool valid() const {
		return _state != nullptr;
	}

	T get() {
		return _state->get();
	}

	bool ready() const {
		if (!valid()) {
			return false;
		}
		return _state->ready();
	}

	void wait() {
		if (!valid()) {
			return;
		}
		_state->wait();
	}
};

namespace detail {

template<typename T>
FutureState<T> *createFutureState() {
	void *mem = core_malloc(sizeof(FutureState<T>));
	return new (mem) FutureState<T>();
}

} // namespace detail

} // namespace core
