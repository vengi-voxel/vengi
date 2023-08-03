/**
 * @file
 */

#pragma once

#include <stddef.h> // size_t

namespace core {

/**
 * @brief Fixed size array - does not allocate
 * @ingroup Collections
 */
template<class TYPE, size_t SIZE>
struct Array {
	TYPE _items[SIZE];

	[[__nodiscard__]] constexpr size_t size() const {
		return SIZE;
	}

	[[__nodiscard__]] const TYPE* data() const {
		return _items;
	}

	[[__nodiscard__]] TYPE* data() {
		return _items;
	}

	[[__nodiscard__]] constexpr bool empty() const {
		return false;
	}

	[[__nodiscard__]] inline constexpr TYPE& operator[](size_t index) {
		return _items[index];
	}

	[[__nodiscard__]] inline constexpr const TYPE& operator[](size_t index) const {
		return _items[index];
	}

	constexpr void fill(const TYPE& value) {
		for (size_t i = 0u; i < SIZE; ++i) {
			_items[i] = value;
		}
	}

	[[__nodiscard__]] constexpr TYPE* begin() {
		return _items;
	}

	[[__nodiscard__]] constexpr const TYPE* begin() const {
		return _items;
	}

	[[__nodiscard__]] constexpr TYPE* end() {
		return &_items[SIZE];
	}

	[[__nodiscard__]] constexpr const TYPE* end() const {
		return &_items[SIZE];
	}
};

}
