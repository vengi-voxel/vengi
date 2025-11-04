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

namespace priv {
struct SharedPtrControlBlock {
	void *_ptr;
	core::AtomicInt _refCnt{1};
};
}

template<class T>
class SharedPtr {
private:
	template <typename U> friend class SharedPtr;

	struct __enableIfHelper {
		int b;
	};

	priv::SharedPtrControlBlock *_ctrl;

	CORE_FORCE_INLINE int count() const {
		if (_ctrl == nullptr) {
			return 0;
		}
		return _ctrl->_refCnt;
	}

	CORE_FORCE_INLINE void increase() {
		if (_ctrl == nullptr) {
			return;
		}
		_ctrl->_refCnt.increment(1);
	}

	CORE_FORCE_INLINE int decrease() {
		if (_ctrl == nullptr) {
			return -1;
		}
		return _ctrl->_refCnt.decrement(1) - 1;
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
			_ctrl(obj._ctrl) {
		increase();
	}

	template <class U>
	SharedPtr(SharedPtr<U> &&obj, typename std::enable_if<std::is_convertible<U*, T*>::value, __enableIfHelper>::type = __enableIfHelper()) :
			_ctrl(obj._ctrl) {
		obj._ctrl = nullptr;
	}

	template<typename ... Args>
	static SharedPtr<T> create(Args&&... args) {
		const size_t alignment = alignof(T);
		const size_t size = sizeof(T) + (alignment - 1) + sizeof(priv::SharedPtrControlBlock);
		void *ptr = core_malloc(size);
		SharedPtr<T> d;
		d._ctrl = new (ptr) priv::SharedPtrControlBlock();
		const uintptr_t aligned = ((uintptr_t)ptr + sizeof(priv::SharedPtrControlBlock) + alignment - 1) & ~(uintptr_t)(alignment - 1);
		d._ctrl->_ptr = new ((void*)(uint8_t*)aligned) T(core::forward<Args>(args)...);
		return d;
	}

	SharedPtr &operator=(const SharedPtr &obj) {
		if (&obj == this || _ctrl == obj._ctrl) {
			return *this;
		}
		release();
		_ctrl = obj._ctrl;
		increase();
		return *this;
	}

	SharedPtr &operator=(SharedPtr &&obj) noexcept {
		if (&obj == this || _ctrl == obj._ctrl) {
			return *this;
		}
		release();
		_ctrl = obj._ctrl;
		obj._ctrl = nullptr;
		return *this;
	}

	~SharedPtr() {
		release();
	}

	core::AtomicInt* refCnt() const {
		return _ctrl ? &_ctrl->_refCnt : nullptr;
	}

	void release() {
		if (decrease() == 0) {
			if (_ctrl != nullptr) {
				((T*)_ctrl->_ptr)->~T();
				_ctrl->~SharedPtrControlBlock();
			}
			core_free((void*)_ctrl);
		}
		_ctrl = nullptr;
	}

	CORE_FORCE_INLINE T *get() const {
		return _ctrl ? (T*)_ctrl->_ptr : nullptr;
	}

	CORE_FORCE_INLINE T *operator->() const {
		return get();
	}

	void operator=(decltype(nullptr)) {
		release();
	}

	CORE_FORCE_INLINE operator bool() const {
		return *this != nullptr;
	}

	CORE_FORCE_INLINE bool operator==(const SharedPtr &rhs) const {
		return _ctrl == rhs._ctrl;
	}

	CORE_FORCE_INLINE bool operator!=(const SharedPtr &rhs) const {
		return _ctrl != rhs._ctrl;
	}

	CORE_FORCE_INLINE bool operator<(const SharedPtr &rhs) const {
		return _ctrl < rhs._ctrl;
	}

	CORE_FORCE_INLINE bool operator>(const SharedPtr &rhs) const {
		return _ctrl > rhs._ctrl;
	}

	CORE_FORCE_INLINE bool operator<=(const SharedPtr &rhs) const {
		return _ctrl <= rhs._ctrl;
	}

	CORE_FORCE_INLINE bool operator>=(const SharedPtr &rhs) const {
		return _ctrl >= rhs._ctrl;
	}

	CORE_FORCE_INLINE bool operator==(decltype(nullptr)) const {
		return nullptr == _ctrl;
	}

	CORE_FORCE_INLINE bool operator!=(decltype(nullptr)) const {
		return nullptr != _ctrl;
	}
};
static_assert(sizeof(SharedPtr<void>) == sizeof(void*), "SharedPtr size must be equal to pointer size");

template<class T, typename ... Args>
static SharedPtr<T> make_shared(Args&&... args) {
	return SharedPtr<T>::create(core::forward<Args>(args)...);
}

}
