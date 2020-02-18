/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include <stdint.h>

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
	inline size_t size() const {
		return _size;
	}

	inline bool empty() const {
		return size() <= 0;
	}

	inline void clear() {
		_size = 0;
	}

	void push(const TYPE &x) {
		core_assert(_size < SIZE);
		_stack[_size++] = x;
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

	inline TYPE &operator[](size_t i) {
		core_assert(i < SIZE);
		return _stack[i];
	}

	inline const TYPE &operator[](size_t i) const {
		core_assert(i < SIZE);
		return _stack[i];
	}
};

}
