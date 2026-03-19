/**
 * @file
 */

#pragma once

#include "core/concurrent/Atomic.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include <new>
#include <stddef.h>

namespace core {

template<class T> class WeakPtr;

namespace priv {
struct SharedPtrControlBlock {
	void *_ptr;
	core::AtomicInt _refCnt{1};
	core::AtomicInt _weakRefCnt{1}; // weak count (+1 while strong refs exist)
};
}

template<class T>
class SharedPtr {
private:
	template <typename U> friend class SharedPtr;
	template <typename U> friend class WeakPtr;

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

	// Decrement the weak reference count and free the control block if it reaches 0
	CORE_FORCE_INLINE void releaseWeak() {
		if (_ctrl == nullptr) {
			return;
		}
		if (_ctrl->_weakRefCnt.decrement(1) - 1 == 0) {
			_ctrl->~SharedPtrControlBlock();
			core_free((void*)_ctrl);
		}
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
	SharedPtr(const SharedPtr<U> &obj, typename core::enable_if<core::is_convertible<U*, T*>::value, int>::type = 0) :
			_ctrl(obj._ctrl) {
		increase();
	}

	template <class U>
	SharedPtr(SharedPtr<U> &&obj, typename core::enable_if<core::is_convertible<U*, T*>::value, int>::type = 0) :
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
				// Release the weak reference that the strong count held
				releaseWeak();
			}
		} else if (_ctrl != nullptr) {
			// Strong count didn't reach zero, nothing more to do
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

/**
 * @brief A non-owning smart pointer that observes an object managed by SharedPtr without
 * preventing its destruction.
 *
 * WeakPtr holds a reference to the control block of a SharedPtr. It does not contribute to
 * the strong reference count, so the managed object can be destroyed while WeakPtr instances
 * exist. Use lock() to obtain a SharedPtr if the object is still alive.
 */
template<class T>
class WeakPtr {
private:
	template <typename U> friend class WeakPtr;

	priv::SharedPtrControlBlock *_ctrl;

	CORE_FORCE_INLINE void increaseWeak() {
		if (_ctrl == nullptr) {
			return;
		}
		_ctrl->_weakRefCnt.increment(1);
	}

	CORE_FORCE_INLINE void releaseWeak() {
		if (_ctrl == nullptr) {
			return;
		}
		if (_ctrl->_weakRefCnt.decrement(1) - 1 == 0) {
			_ctrl->~SharedPtrControlBlock();
			core_free((void*)_ctrl);
		}
		_ctrl = nullptr;
	}

public:
	constexpr WeakPtr() : _ctrl(nullptr) {
	}

	constexpr WeakPtr(decltype(nullptr)) : _ctrl(nullptr) {
	}

	WeakPtr(const SharedPtr<T> &shared) : _ctrl(shared._ctrl) {
		increaseWeak();
	}

	template <class U>
	WeakPtr(const SharedPtr<U> &shared, typename core::enable_if<core::is_convertible<U*, T*>::value, int>::type = 0) :
			_ctrl(shared._ctrl) {
		increaseWeak();
	}

	WeakPtr(const WeakPtr &obj) : _ctrl(obj._ctrl) {
		increaseWeak();
	}

	WeakPtr(WeakPtr &&obj) noexcept : _ctrl(obj._ctrl) {
		obj._ctrl = nullptr;
	}

	template <class U>
	WeakPtr(const WeakPtr<U> &obj, typename core::enable_if<core::is_convertible<U*, T*>::value, int>::type = 0) :
			_ctrl(obj._ctrl) {
		increaseWeak();
	}

	template <class U>
	WeakPtr(WeakPtr<U> &&obj, typename core::enable_if<core::is_convertible<U*, T*>::value, int>::type = 0) :
			_ctrl(obj._ctrl) {
		obj._ctrl = nullptr;
	}

	~WeakPtr() {
		releaseWeak();
	}

	WeakPtr &operator=(const WeakPtr &obj) {
		if (&obj == this || _ctrl == obj._ctrl) {
			return *this;
		}
		releaseWeak();
		_ctrl = obj._ctrl;
		increaseWeak();
		return *this;
	}

	WeakPtr &operator=(WeakPtr &&obj) noexcept {
		if (&obj == this || _ctrl == obj._ctrl) {
			return *this;
		}
		releaseWeak();
		_ctrl = obj._ctrl;
		obj._ctrl = nullptr;
		return *this;
	}

	WeakPtr &operator=(const SharedPtr<T> &shared) {
		releaseWeak();
		_ctrl = shared._ctrl;
		increaseWeak();
		return *this;
	}

	void operator=(decltype(nullptr)) {
		releaseWeak();
	}

	/**
	 * @brief Check if the managed object has been destroyed.
	 * @return true if the object has expired (strong ref count is 0), false otherwise.
	 */
	bool expired() const {
		return _ctrl == nullptr || (int)_ctrl->_refCnt == 0;
	}

	/**
	 * @brief Attempt to obtain a SharedPtr to the managed object.
	 *
	 * Atomically checks whether the object is still alive and, if so, increments the
	 * strong reference count.
	 *
	 * @return A SharedPtr to the object, or an empty SharedPtr if the object has expired.
	 */
	SharedPtr<T> lock() const {
		if (_ctrl == nullptr) {
			return SharedPtr<T>();
		}
		// Atomically try to increment the strong ref count, but only if it's > 0
		for (;;) {
			int current = (int)_ctrl->_refCnt;
			if (current <= 0) {
				return SharedPtr<T>();
			}
			if (_ctrl->_refCnt.compare_exchange(current, current + 1)) {
				SharedPtr<T> result;
				result._ctrl = _ctrl;
				// We already incremented the strong count, so don't call increase()
				return result;
			}
			// CAS failed, another thread changed _refCnt, retry
		}
	}

	void reset() {
		releaseWeak();
	}

	CORE_FORCE_INLINE bool operator==(const WeakPtr &rhs) const {
		return _ctrl == rhs._ctrl;
	}

	CORE_FORCE_INLINE bool operator!=(const WeakPtr &rhs) const {
		return _ctrl != rhs._ctrl;
	}

	CORE_FORCE_INLINE bool operator==(decltype(nullptr)) const {
		return nullptr == _ctrl;
	}

	CORE_FORCE_INLINE bool operator!=(decltype(nullptr)) const {
		return nullptr != _ctrl;
	}
};
static_assert(sizeof(WeakPtr<void>) == sizeof(void*), "WeakPtr size must be equal to pointer size");

}
