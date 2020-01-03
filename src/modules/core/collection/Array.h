/**
 * @file
 */

#pragma once

#include "core/Assert.h"

namespace core {

template<class TYPE, size_t SIZE>
struct Array {
	TYPE _items[SIZE];

	size_t size() const {
		return SIZE;
	}

	inline TYPE& operator[](size_t index) {
		core_assert(index < SIZE);
		return _items[index];
	}

	inline const TYPE& operator[](size_t index) const {
		core_assert(index < SIZE);
		return _items[index];
	}

	void fill(const TYPE& value) {
		for (size_t i = 0u; i < SIZE; ++i) {
			_items[i] = value;
		}
	}

	TYPE* begin() {
		return _items;
	}

	const TYPE* begin() const {
		return _items;
	}

	TYPE* end() {
		return &_items[SIZE];
	}

	const TYPE* end() const {
		return &_items[SIZE];
	}
};

}
