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
			if (i < _size) {
				_items[i].~TYPE();
			}
			_items[i] = value;
		}
		_size = SIZE;
	}

	constexpr void assign(const TYPE& value, size_t n) {
		for (size_t i = 0u; i < n; ++i) {
			if (i < _size) {
				_items[i].~TYPE();
			}
			_items[i] = value;
		}
		_size = core_max(_size, n);
	}

	template<typename... _Args>
	bool emplace_back(_Args&&... args) {
		if (_size >= SIZE) {
			return false;
		}
		(*this)[_size++] = TYPE(core::forward<_Args>(args)...);
		return true;
	}

	bool push_back(const TYPE& value) {
		if (_size >= SIZE) {
			return false;
		}
		(*this)[_size++] = value;
		return true;
	}

	const TYPE& front() const {
		return _items[0];
	}

	TYPE& front() {
		return _items[0];
	}

	constexpr TYPE* begin() {
		return _items;
	}

	void clear() {
		for (size_t i = 0; i < _size; ++i) {
			_items[i].~TYPE();
		}
		_size = 0;
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
