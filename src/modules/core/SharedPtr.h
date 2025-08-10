/**
 * @file
 */

#pragma once

#include "core/concurrent/Atomic.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include <new>
#include <type_traits>
#include <stddef.h>

namespace core {

template<class T>
class SharedPtr {
private:
	template <typename U> friend class SharedPtr;

	struct __enableIfHelper {
		int b;
	};

	// ControlBlock holds ref count + object storage
	struct ControlBlock {
		core::AtomicInt refCnt;
		typename std::aligned_storage<sizeof(T), alignof(T)>::type object;
	};

	ControlBlock* _ctrl;

	int count() const {
		if (_ctrl == nullptr) {
			return 0;
		}
		return _ctrl->refCnt;
	}

	void increase() {
		if (_ctrl == nullptr) {
			return;
		}
		_ctrl->refCnt.increment(1);
	}

	int decrease() {
		if (_ctrl == nullptr) {
			return -1;
		}
		return _ctrl->refCnt.decrement(1) - 1;
	}

public:
	using value_type = T;

	constexpr SharedPtr() : _ctrl(nullptr) {
	}

	constexpr SharedPtr(decltype(nullptr)) : _ctrl(nullptr) {
	}

	SharedPtr(const SharedPtr &obj) : _ctrl(obj._ctrl) {
		increase();
	}

	SharedPtr(SharedPtr &&obj) noexcept : _ctrl(obj._ctrl) {
		obj._ctrl = nullptr;
	}

	template <class U>
	SharedPtr(const SharedPtr<U> &obj, typename std::enable_if<std::is_convertible<U*, T*>::value, __enableIfHelper>::type = __enableIfHelper()) :
		_ctrl(reinterpret_cast<ControlBlock*>(obj._ctrl)) {
		increase();
	}

	template <class U>
	SharedPtr(SharedPtr<U> &&obj, typename std::enable_if<std::is_convertible<U*, T*>::value, __enableIfHelper>::type = __enableIfHelper()) :
		_ctrl(reinterpret_cast<ControlBlock*>(obj._ctrl)) {
		obj._ctrl = nullptr;
	}

	template<typename ... Args>
	static SharedPtr<T> create(Args&&... args) {
		void *mem = core_malloc(sizeof(ControlBlock));
		ControlBlock *ctrl = static_cast<ControlBlock *>(mem);
		new (&ctrl->refCnt) core::AtomicInt(1);
		new (&ctrl->object) T(core::forward<Args>(args)...);
		SharedPtr<T> d;
		d._ctrl = ctrl;
		return d;
	}

	SharedPtr &operator=(const SharedPtr &obj) {
		if (&obj == this) {
			return *this;
		}
		release();
		_ctrl = obj._ctrl;
		increase();
		return *this;
	}

	SharedPtr &operator=(SharedPtr &&obj) noexcept {
		if (&obj == this) {
			return *this;
		}
		release();
		_ctrl = obj._ctrl;
		obj._ctrl = nullptr;
		return *this;
	}

	// templated copy assignment for convertible types
	template <class U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value, __enableIfHelper>::type>
	SharedPtr &operator=(const SharedPtr<U> &obj) {
		release();
		_ctrl = reinterpret_cast<ControlBlock*>(obj._ctrl);
		increase();
		return *this;
	}

	// templated move assignment for convertible types
	template <class U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value, __enableIfHelper>::type>
	SharedPtr &operator=(SharedPtr<U> &&obj) {
		release();
		_ctrl = reinterpret_cast<ControlBlock*>(obj._ctrl);
		obj._ctrl = nullptr;
		return *this;
	}

	~SharedPtr() {
		release();
	}

	core::AtomicInt* refCnt() const {
		return _ctrl ? &_ctrl->refCnt : nullptr;
	}

	void release() {
		if (decrease() == 0) {
			if (_ctrl != nullptr) {
				reinterpret_cast<T*>(&_ctrl->object)->~T();
				_ctrl->~ControlBlock();
				core_free(_ctrl);
			}
		}
		_ctrl = nullptr;
	}

	inline T *get() const {
		return _ctrl ? reinterpret_cast<T*>(&_ctrl->object) : nullptr;
	}

	inline T *operator->() const {
		return get();
	}

	void operator=(decltype(nullptr)) {
		release();
	}

	inline operator bool() const {
		return get() != nullptr;
	}

	inline bool operator==(const SharedPtr &rhs) const {
		return get() == rhs.get();
	}

	inline bool operator!=(const SharedPtr &rhs) const {
		return get() != rhs.get();
	}

	inline bool operator<(const SharedPtr &rhs) const {
		return get() < rhs.get();
	}

	inline bool operator>(const SharedPtr &rhs) const {
		return get() > rhs.get();
	}

	inline bool operator<=(const SharedPtr &rhs) const {
		return get() <= rhs.get();
	}

	inline bool operator>=(const SharedPtr &rhs) const {
		return get() >= rhs.get();
	}

	inline bool operator==(decltype(nullptr)) const {
		return get() == nullptr;
	}

	inline bool operator!=(decltype(nullptr)) const {
		return get() != nullptr;
	}
};
static_assert(sizeof(SharedPtr<int>) == sizeof(void *), "SharedPtr size unexpected");

template<class T, typename ... Args>
static SharedPtr<T> make_shared(Args&&... args) {
	return SharedPtr<T>::create(core::forward<Args>(args)...);
}

} // namespace core
