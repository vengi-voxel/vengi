/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/Common.h"
#include <stddef.h>

namespace core {

// TODO: rename to DynamicQueue
/**
 * A queue class based on a ring buffer implementation.
 * @ingroup Collections
 */
template<class TYPE, size_t INCREASE = 32u>
class Queue {
private:
	TYPE *_buffer = nullptr;
	size_t _capacity = 0;
	size_t _head = 0;
	size_t _tail = 0;
	size_t _size = 0;

	void grow() {
		size_t newCapacity = (_capacity == 0) ? INCREASE : _capacity + INCREASE;
		TYPE *newBuffer = new TYPE[newCapacity];

		// Copy elements from old buffer to new one
		for (size_t i = 0; i < _size; ++i) {
			newBuffer[i] = core::move(_buffer[(_head + i) % _capacity]);
		}

		delete[] _buffer;
		_buffer = newBuffer;
		_capacity = newCapacity;
		_head = 0;
		_tail = _size;
	}

public:
	Queue() = default;

	~Queue() {
		delete[] _buffer;
	}

	bool empty() const {
		return _size == 0;
	}

	void clear() {
		_size = 0;
		_head = 0;
		_tail = 0;
	}

	void reserve(size_t size) {
		if (size > _capacity) {
			size_t newCapacity = size;
			TYPE *newBuffer = new TYPE[newCapacity];
			for (size_t i = 0; i < _size; ++i) {
				newBuffer[i] = core::move(_buffer[(_head + i) % _capacity]);
			}
			delete[] _buffer;
			_buffer = newBuffer;
			_capacity = newCapacity;
			_head = 0;
			_tail = _size;
		}
	}

	void push(const TYPE &value) {
		if (_size == _capacity) {
			grow();
		}
		_buffer[_tail] = value;
		_tail = (_tail + 1) % _capacity;
		++_size;
	}

	void push(TYPE &&value) {
		if (_size == _capacity) {
			grow();
		}
		_buffer[_tail] = core::move(value);
		_tail = (_tail + 1) % _capacity;
		++_size;
	}

	template<typename... Args>
	void emplace(Args &&...args) {
		if (_size == _capacity) {
			grow();
		}
		new (&_buffer[_tail]) TYPE(core::forward<Args>(args)...);
		_tail = (_tail + 1) % _capacity;
		++_size;
	}

	TYPE pop() {
		core_assert_msg(!empty(), "Cannot pop from empty queue");
		TYPE result = core::move(_buffer[_head]);
		_head = (_head + 1) % _capacity;
		--_size;
		return result;
	}

	bool try_pop(TYPE &out) {
		if (empty()) {
			return false;
		}
		out = core::move(_buffer[_head]);
		_head = (_head + 1) % _capacity;
		--_size;
		return true;
	}

	CORE_FORCE_INLINE TYPE &front() {
		core_assert(!empty());
		return _buffer[_head];
	}

	CORE_FORCE_INLINE const TYPE &front() const {
		core_assert(!empty());
		return _buffer[_head];
	}

	CORE_FORCE_INLINE TYPE &back() {
		core_assert(!empty());
		return _buffer[(_tail + _capacity - 1) % _capacity];
	}

	CORE_FORCE_INLINE const TYPE &back() const {
		core_assert(!empty());
		return _buffer[(_tail + _capacity - 1) % _capacity];
	}

	CORE_FORCE_INLINE size_t size() const {
		return _size;
	}

	CORE_FORCE_INLINE size_t capacity() const {
		return _capacity;
	}
};

} // namespace core
