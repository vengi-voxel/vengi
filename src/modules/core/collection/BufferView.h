/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include <stdint.h>
#include <string.h>

namespace core {

/**
 * @brief Access to a C style array for a particular type
 * @ingroup Collections
 */
template<typename TYPE>
class BufferView {
private:
	const TYPE *_buffer;
	size_t _size;

public:
	using value_type = TYPE;

	BufferView(const TYPE *buffer, size_t elements) : _buffer(buffer), _size(elements) {
	}

	BufferView(const TYPE *buffer, size_t begin, size_t end) : _buffer(buffer + begin), _size(end - begin) {
		core_assert(end > begin);
	}

	inline BufferView<TYPE> sub(size_t from, size_t len) const {
		core_assert(from + len <= _size);
		return BufferView<TYPE>(_buffer + from, len);
	}

	inline BufferView<TYPE> sub(size_t from) const {
		core_assert(from < _size);
		return BufferView<TYPE>(_buffer + from, _size - from);
	}

	class iterator {
	private:
		const TYPE *_ptr;

	public:
		constexpr iterator() : _ptr(nullptr) {
		}

		iterator(const TYPE *ptr) : _ptr(ptr) {
		}

		inline const TYPE &operator*() const {
			return *_ptr;
		}

		iterator &operator++() {
			++_ptr;
			return *this;
		}

		iterator operator++(int) {
			return iterator(_ptr++);
		}

		iterator operator--(int) {
			return iterator(_ptr--);
		}

		int operator-(iterator rhs) const {
			return (int)(intptr_t)(_ptr - rhs._ptr);
		}

		iterator &operator+(size_t n) {
			_ptr += n;
			return *this;
		}

		iterator &operator+=(size_t n) {
			_ptr += n;
			return *this;
		}

		iterator &operator--() {
			--_ptr;
			return *this;
		}

		iterator &operator-(size_t n) {
			_ptr -= n;
			return *this;
		}

		iterator &operator-=(size_t n) {
			_ptr -= n;
			return *this;
		}

		inline const TYPE *operator->() const {
			return _ptr;
		}

		inline bool operator!=(const iterator &rhs) const {
			return _ptr != rhs._ptr;
		}

		inline bool operator==(const iterator &rhs) const {
			return _ptr == rhs._ptr;
		}
	};
	using const_iterator = iterator;

	inline bool empty() const {
		return _size == 0u;
	}

	const TYPE *data() const {
		return _buffer;
	}

	TYPE &front() {
		core_assert(_size > 0u);
		return _buffer[0];
	}

	const TYPE &front() const {
		core_assert(_size > 0u);
		return _buffer[0];
	}

	TYPE &back() {
		core_assert(_size > 0u);
		return _buffer[_size - 1u];
	}

	inline const TYPE &operator[](size_t idx) const {
		core_assert(idx < _size);
		return _buffer[idx];
	}

	inline size_t size() const {
		return _size;
	}

	inline iterator begin() const {
		return iterator(_buffer);
	}

	inline iterator end() const {
		return iterator(_buffer + _size);
	}
};

} // namespace core
