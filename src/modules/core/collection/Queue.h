/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
namespace core {

/**
 * A queue class based on @c DynamicArray
 * @ingroup Collections
 */
template<class TYPE, size_t INCREASE = 32u>
class Queue {
public:
	using IMPL = core::DynamicArray<TYPE, INCREASE>;
private:
	IMPL _array;
public:
	bool empty() const {
		return _array.empty();
	}

	void clear() {
		_array.clear();
	}

	void reserve(size_t size) {
		_array.reserve(size);
	}

	void resize(size_t size) {
		_array.resize(size);
	}

	typename IMPL::iterator begin() const {
		return _array.begin();
	}

	typename IMPL::iterator end() const {
		return _array.end();
	}

	template<typename... Args>
	void emplace(Args&&... args) {
		_array.emplace_back(core::forward<Args>(args)...);
	}

	void push(const TYPE &x) {
		_array.push_back(x);
	}

	TYPE &front() {
		return _array.front();
	}

	const TYPE &front() const {
		return _array.front();
	}

	TYPE &back() {
		return _array.back();
	}

	const TYPE &back() const {
		return _array.back();
	}

	TYPE pop() {
		const TYPE tmp = front();
		_array.erase(_array.begin());
		return tmp;
	}

	bool try_pop(TYPE& out) {
		if (empty()) {
			return false;
		}
		out = front();
		_array.erase(_array.begin());
		return true;
	}

	inline size_t size() const {
		return _array.size();
	}

	inline size_t capacity() const {
		return _array.capacity();
	}
};

} // namespace core
