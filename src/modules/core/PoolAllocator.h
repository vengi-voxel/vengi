/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/Common.h"
#include "core/NonCopyable.h"
#include "core/StandardLib.h"
#include <stdint.h>
#include <type_traits>
#include <new>
#ifdef _WIN32
#undef max
#endif

namespace core {

/**
 * @brief Allocator for a fixed amount of objects. The used memory can not grow or shrink.
 */
template<typename T, typename SIZE = uint16_t>
class PoolAllocator : public core::NonCopyable {
public:
	using Type = T;
	using PointerType = T*;
	using SizeType = SIZE;
private:
	static constexpr Type* POOLBUFFER_END_MARKER = nullptr;
	static_assert(sizeof(T) >= sizeof(T*), "T must at least be of the same size as T*");
	// the pool buffer memory
	Type* _poolBuf = nullptr;
	// fast lookup of the next free slot in the pool
	Type* _nextFreeSlot = nullptr;
	SizeType _maxPoolSize = (SizeType)0;
	SizeType _currentAllocatedItems = (SizeType)0;

	template<class ... Args>
	void callConstructor(std::false_type, T *ptr, Args&& ...) {
	}

	template<class ... Args>
	void callConstructor(std::true_type, T *ptr, Args&& ...args) {
		new (ptr)Type(core::forward<Args>(args) ... );
	}

	void callDestructor(std::false_type, T *ptr) {
	}

	void callDestructor(std::true_type, T *ptr) {
		ptr->~T();
	}

	inline bool outOfRange(Type* ptr) const {
		return ptr < &_poolBuf[0] || ptr > &_poolBuf[_maxPoolSize - 1];
	}
public:
	~PoolAllocator() {
		core_assert_msg(_poolBuf == nullptr, "PoolAllocator wasn't shut down properly");
		shutdown();
	}

	bool init(SizeType poolSize) {
		if (_poolBuf != nullptr) {
			if (_maxPoolSize == poolSize + 1) {
				return true;
			}
			return false;
		}

		if (poolSize <= 1) {
			return false;
		}

		_maxPoolSize = poolSize;
		_poolBuf = (Type*)core_malloc(sizeof(T) * _maxPoolSize);
		for (SizeType i = (SizeType)0; i < _maxPoolSize - 1; ++i) {
			// init the next free slot address (for fast lookup)
			*(Type**) &_poolBuf[i] = &_poolBuf[i + 1];
		}

		// put end of list marker to ... the end of the list.
		*(Type**) &_poolBuf[_maxPoolSize - 1] = POOLBUFFER_END_MARKER;

		_nextFreeSlot = _poolBuf;
		_currentAllocatedItems = (SizeType)0;

		return true;
	}

	void shutdown() {
		core_assert_msg(_currentAllocatedItems == 0, "There are still %i items left", _currentAllocatedItems);
		core_free(_poolBuf);
		_poolBuf = nullptr;
		_nextFreeSlot = nullptr;
		_maxPoolSize = (SizeType)0;
		_currentAllocatedItems = (SizeType)0;
	}

	inline SizeType allocated() const {
		return _currentAllocatedItems;
	}

	inline SizeType max() const {
		return _maxPoolSize;
	}

	T* alloc() {
		Type* ptr = nullptr;
		if (_nextFreeSlot != POOLBUFFER_END_MARKER) {
			core_assert_msg(_currentAllocatedItems < (this->max)(), "Exceed the max allowed items while the end of slot marker wasn't found");
			core_assert_msg(!outOfRange(_nextFreeSlot), "Out of range after %i allocated slots", (int)_currentAllocatedItems);
			ptr = _nextFreeSlot;
			_nextFreeSlot = *(Type**)ptr;
			++_currentAllocatedItems;
			core_assert_msg(_nextFreeSlot == POOLBUFFER_END_MARKER || !outOfRange(_nextFreeSlot), "Out of range after %i allocated slots", (int)_currentAllocatedItems);
			callConstructor(std::is_class<T> {}, ptr);
		}

		return ptr;
	}

	template<class ... Args>
	inline T* alloc(Args&&... args) {
		Type* ptr = nullptr;
		if (_nextFreeSlot != POOLBUFFER_END_MARKER) {
			core_assert_msg(_currentAllocatedItems < (this->max)(), "Exceed the max allowed items while the end of slot marker wasn't found");
			core_assert_msg(!outOfRange(_nextFreeSlot), "Out of range after %i allocated slots", (int)_currentAllocatedItems);
			ptr = _nextFreeSlot;
			_nextFreeSlot = *(Type**)ptr;
			++_currentAllocatedItems;
			core_assert_msg(_nextFreeSlot == POOLBUFFER_END_MARKER || !outOfRange(_nextFreeSlot), "Out of range after %i allocated slots", (int)_currentAllocatedItems);
			callConstructor(std::is_class<T> {}, ptr, std::forward<Args>(args) ...);
		}

		return ptr;
	}

	bool free(T* ptr) {
		if (ptr == nullptr) {
			return false;
		}

		if (outOfRange(ptr)) {
			return false;
		}

		core_assert(_currentAllocatedItems > 0);
		callDestructor(std::is_class<T> {}, ptr);
		*(Type**) ptr = _nextFreeSlot;
		_nextFreeSlot = ptr;
		--_currentAllocatedItems;
		return true;
	}
};

}
