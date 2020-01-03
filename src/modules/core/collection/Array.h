/**
 * @file
 */

#pragma once

#include "core/Assert.h"

namespace core {

template<class TYPE, int SIZE>
class Array {
private:
	TYPE _items[SIZE];
public:
	int size() const {
		return SIZE;
	}

	TYPE& operator[](int index) {
		core_assert(index >= 0 && index < SIZE);
		return _items[index];
	}

	const TYPE& operator[](int index) const {
		core_assert(index >= 0 && index < SIZE);
		return _items[index];
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
