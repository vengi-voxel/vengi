/**
 * @file
 */

#pragma once

#include "Assert.h"
#include "Log.h"
#include <stdint.h>
#include <type_traits>
#include <SDL.h>
#ifdef _WIN32
#undef max
#endif

namespace core {

/**
 * @brief Allocator for a fixed amount of objects. The used memory can not grow or shrink.
 */
template<typename T, typename SIZE = uint16_t>
class PoolAllocator {
public:
	using Type = T;
	using PointerType = Type*;
	using SizeType = SIZE;
private:
	static constexpr PointerType POOLBUFFER_END_MARKER = nullptr;
	// the pool buffer memory
	PointerType _poolBuf = nullptr;
	// fast lookup of the next free slot in the pool
	PointerType _nextFreeSlot = nullptr;
	SizeType _maxPoolSize = (SizeType)0;
	SizeType _currentAllocatedItems = (SizeType)0;

	template<class ... Args>
	void callConstructor(std::false_type, T *ptr, Args&& ...) {
	}

	template<class ... Args>
	void callConstructor(std::true_type, T *ptr, Args&& ...args) {
		new (ptr)Type(std::forward<Args>(args) ... );
	}

	void callDestructor(std::false_type, T *ptr) {
	}

	void callDestructor(std::true_type, T *ptr) {
		ptr->~T();
	}

	inline bool outOfRange(PointerType ptr) const {
		return ptr < &_poolBuf[0] || ptr > &_poolBuf[_maxPoolSize - 1];
	}
public:
	~PoolAllocator() {
		core_assert_msg(_poolBuf == nullptr, "PoolAllocator wasn't shut down properly");
		shutdown();
	}

	bool init(SizeType poolSize) {
		if (_poolBuf != nullptr) {
			if (_maxPoolSize == poolSize) {
				Log::debug("Pool is already initialized - don't do anything in this init() call");
				return true;
			}
			Log::error("Pool is already initialized");
			return false;
		}

		if (poolSize <= 1) {
			Log::error("Pool buffer is not big enough");
			return false;
		}

		_poolBuf = (PointerType)SDL_malloc(sizeof(T) * poolSize);
		for (SizeType i = (SizeType)0; i < poolSize - 1; ++i) {
			// init the next free slot address (for fast lookup)
			*(PointerType*) &_poolBuf[i] = &_poolBuf[i + 1];
		}

		// put end of list marker to ... the end of the list.
		*(PointerType*) &_poolBuf[poolSize - 1] = POOLBUFFER_END_MARKER;

		_nextFreeSlot = _poolBuf;
		_maxPoolSize = poolSize;
		_currentAllocatedItems = (SizeType)0;

		return true;
	}

	void shutdown() {
		core_assert_msg(_currentAllocatedItems == 0, "There are still %i items left", _currentAllocatedItems);
		SDL_free(_poolBuf);
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
		PointerType ptr = nullptr;
		if (_nextFreeSlot != POOLBUFFER_END_MARKER) {
			core_assert(_currentAllocatedItems < _maxPoolSize);
			core_assert(!outOfRange(_nextFreeSlot));
			ptr = _nextFreeSlot;
			callConstructor(std::is_class<T> {}, ptr);
			_nextFreeSlot = *(PointerType*) ptr;
			++_currentAllocatedItems;
		}

		return ptr;
	}

	template<class ... Args>
	inline T* alloc(Args&&... args) {
		PointerType ptr = nullptr;
		if (_nextFreeSlot != POOLBUFFER_END_MARKER) {
			core_assert(_currentAllocatedItems < _maxPoolSize);
			core_assert(!outOfRange(_nextFreeSlot));
			ptr = _nextFreeSlot;
			callConstructor(std::is_class<T> {}, ptr, std::forward<Args>(args) ...);
			_nextFreeSlot = *(PointerType*) ptr;
			++_currentAllocatedItems;
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
		*(PointerType*) ptr = _nextFreeSlot;
		_nextFreeSlot = ptr;
		--_currentAllocatedItems;
		return true;
	}
};

}
