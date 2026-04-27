/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include <stddef.h>

namespace core {

/**
 * @brief Most-recently-used buffer with a fixed capacity that avoids duplicates.
 *
 * When an element is added that already exists, it is moved to the front (most recent position).
 * When the buffer is full, the least recently used element is evicted.
 * Element 0 is always the most recently used.
 *
 * @ingroup Collections
 */
template <typename TYPE, size_t SIZE = 10u>
class MRUBuffer {
private:
	size_t _size = 0u;
	TYPE _buffer[SIZE]{};

public:
	using value_type = TYPE;

	class iterator {
	private:
		const MRUBuffer *_buf;
		size_t _idx;

	public:
		iterator(const MRUBuffer *buf, size_t idx) : _buf(buf), _idx(idx) {
		}

		CORE_FORCE_INLINE const TYPE &operator*() const {
			return _buf->_buffer[_idx];
		}

		CORE_FORCE_INLINE iterator &operator++() {
			++_idx;
			return *this;
		}

		CORE_FORCE_INLINE bool operator!=(const iterator &rhs) const {
			return _idx != rhs._idx;
		}

		CORE_FORCE_INLINE bool operator==(const iterator &rhs) const {
			return _idx == rhs._idx;
		}
	};

	iterator begin() const {
		return iterator(this, 0);
	}

	iterator end() const {
		return iterator(this, _size);
	}

	CORE_FORCE_INLINE size_t size() const {
		return _size;
	}

	constexpr size_t capacity() const {
		return SIZE;
	}

	CORE_FORCE_INLINE bool empty() const {
		return _size == 0;
	}

	void clear() {
		_size = 0u;
	}

	/**
	 * @brief Add an element as the most recently used.
	 *
	 * If the element already exists, it is moved to the front.
	 * If the buffer is full, the least recently used element is evicted.
	 */
	void push(const TYPE &val) {
		// check if already present
		for (size_t i = 0; i < _size; ++i) {
			if (_buffer[i] == val) {
				// shift elements to fill the gap and place val at front
				for (size_t j = i; j > 0; --j) {
					_buffer[j] = _buffer[j - 1];
				}
				_buffer[0] = val;
				return;
			}
		}
		// not found - insert at front
		const size_t end = _size < SIZE ? _size : SIZE - 1;
		for (size_t j = end; j > 0; --j) {
			_buffer[j] = _buffer[j - 1];
		}
		_buffer[0] = val;
		if (_size < SIZE) {
			++_size;
		}
	}

	CORE_FORCE_INLINE const TYPE &operator[](size_t i) const {
		core_assert(i < _size);
		return _buffer[i];
	}
};

} // namespace core
