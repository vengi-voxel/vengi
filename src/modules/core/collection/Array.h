/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include <stddef.h> // size_t

namespace core {

/**
 * @brief Fixed size array - does not allocate
 * @ingroup Collections
 */
template<class TYPE, size_t SIZE>
struct Array {
	TYPE _items[SIZE];

	constexpr size_t size() const {
		return SIZE;
	}

	constexpr bool empty() const {
		return false;
	}

	inline TYPE& operator[](size_t index) {
		core_assert(index < SIZE);
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
	}

	constexpr TYPE* begin() {
		return _items;
	}

	constexpr const TYPE* begin() const {
		return _items;
	}

	constexpr TYPE* end() {
		return &_items[SIZE];
	}

	constexpr const TYPE* end() const {
		return &_items[SIZE];
	}
};

}
