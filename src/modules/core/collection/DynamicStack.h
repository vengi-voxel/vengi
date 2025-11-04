/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/collection/DynamicArray.h"
#include <stddef.h>

namespace core {

/**
 * @brief Dynamic sized stack implementation - does allocate
 *
 * @sa Stack class for a non-allocating implementation
 * @ingroup Collections
 */
template<class TYPE>
class DynamicStack {
private:
	core::DynamicArray<TYPE> _stack;

public:
	using value_type = TYPE;

	CORE_FORCE_INLINE size_t size() const {
		return _stack.size();
	}

	CORE_FORCE_INLINE bool empty() const {
		return _stack.empty();
	}

	CORE_FORCE_INLINE void clear() {
		_stack.clear();
	}

	void push(const TYPE &x) {
		_stack.push_back(x);
	}

	template<typename... _Args>
	void emplace(_Args &&...args) {
		_stack.emplace_back(core::forward<_Args>(args)...);
	}

	const TYPE &top() const {
		return _stack.back();
	}

	TYPE &top() {
		return _stack.back();
	}

	TYPE pop() {
		TYPE tmp = top();
		_stack.pop();
		return tmp;
	}

	CORE_FORCE_INLINE TYPE &operator[](size_t i) {
		return _stack[i];
	}

	CORE_FORCE_INLINE const TYPE &operator[](size_t i) const {
		return _stack[i];
	}
};

} // namespace core
