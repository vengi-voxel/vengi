/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Assert.h"
#include <string.h>

namespace core {

/**
 * @brief Fixed size vector - does not allocate - but maintains the element amount that was added to it
 * @ingroup Collections
 */
template<class TYPE, size_t SIZE>
class Vector {
protected:
	TYPE _items[SIZE];
	size_t _size = 0;
public:
	using value_type = TYPE;

	inline size_t size() const {
		return _size;
	}

	inline bool empty() const {
		return _size == 0u;
	}

	constexpr size_t capacity() const {
		return SIZE;
	}

	inline TYPE& operator[](size_t index) {
		core_assert(index < SIZE);
		_size = core_max(_size, index + 1);
		return _items[index];
	}

	inline const TYPE& operator[](size_t index) const {
		core_assert(index < SIZE);
		return _items[index];
	}

	constexpr void fill(const TYPE& value) {
		for (size_t i = 0u; i < SIZE; ++i) {
			_items[i] = value;
		}
		_size = SIZE;
	}

	template<typename... _Args>
	void emplace_back(_Args&&... args) {
		(*this)[_size] = TYPE(core::forward<_Args>(args)...);
	}

	void push_back(const TYPE& value) {
		(*this)[_size] = value;
	}

	constexpr TYPE* begin() {
		return _items;
	}

	constexpr const TYPE* begin() const {
		return _items;
	}

	constexpr TYPE* end() {
		return &_items[_size];
	}

	constexpr const TYPE* end() const {
		return &_items[_size];
	}
};

}
