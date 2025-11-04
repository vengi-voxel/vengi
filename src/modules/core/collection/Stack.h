/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/Common.h"
#include <stddef.h>

namespace core {

/**
 * @brief Fixed size stack implementation - does not allocate
 *
 * @ingroup Collections
 */
template <class TYPE, size_t SIZE>
class Stack {
private:
	size_t _size = 0u;
	TYPE _stack[SIZE];

public:
	using value_type = TYPE;

	constexpr size_t maxSize() const {
		return SIZE;
	}

	CORE_FORCE_INLINE size_t size() const {
		return _size;
	}

	CORE_FORCE_INLINE bool empty() const {
		return size() <= 0;
	}

	CORE_FORCE_INLINE void clear() {
		_size = 0;
	}

	void push(const TYPE &x) {
		core_assert(_size < SIZE);
		_stack[_size++] = x;
	}

	template<typename... _Args>
	void emplace(_Args&&... args) {
		core_assert(_size < SIZE);
		_stack[_size++] = TYPE(core::forward<_Args>(args)...);
	}

	const TYPE &top() const {
		core_assert(_size > 0);
		return _stack[_size - 1];
	}

	TYPE &top() {
		core_assert(_size > 0);
		return _stack[_size - 1];
	}

	TYPE pop() {
		TYPE tmp = top();
		--_size;
		return tmp;
	}

	CORE_FORCE_INLINE TYPE &operator[](size_t i) {
		core_assert(i < SIZE);
		return _stack[i];
	}

	CORE_FORCE_INLINE const TYPE &operator[](size_t i) const {
		core_assert(i < SIZE);
		return _stack[i];
	}
};

}
