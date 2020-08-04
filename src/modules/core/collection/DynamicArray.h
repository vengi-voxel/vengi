/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include <new>

namespace core {

/**
 * @brief Dynamically growing continuous storage buffer
 *
 * @note This array does not have an upper size limit. Each time the capacity is reached, it will
 * allocate new slots given by the @c INCREASE template parameter.
 *
 * @note Use a fixed size array to prevent memory allocations - where possible
 * @sa Array
 * @ingroup Collections
 */
template<class TYPE, size_t INCREASE = 32u>
class DynamicArray {
private:
	TYPE* _buffer = nullptr;
	size_t _capacity = 0u;
	size_t _size = 0u;

	static inline constexpr size_t align(size_t val) {
		const size_t len = INCREASE - 1u;
		return (size_t)((val + len) & ~len);
	}

	void checkBufferSize(size_t newSize) {
		if (_capacity >= newSize) {
			return;
		}
		_capacity = align(newSize);
		TYPE* newBuffer = (TYPE*)core_malloc(_capacity * sizeof(TYPE));
		for (size_t i = 0u; i < _size; ++i) {
			new ((void*)&newBuffer[i]) TYPE(core::move(_buffer[i]));
			_buffer[i].~TYPE();
		}
		core_free(_buffer);
		_buffer = newBuffer;
	}
public:
	using value_type = TYPE;

	DynamicArray() {
	}

	explicit DynamicArray(size_t amount) {
		checkBufferSize(amount);
		_size = amount;
		for (size_t i = 0u; i < _size; ++i) {
			new (&_buffer[i]) TYPE();
		}
	}

	DynamicArray(const DynamicArray& other) {
		checkBufferSize(other._size);
		_size = other._size;
		for (size_t i = 0u; i < _size; ++i) {
			new (&_buffer[i]) TYPE(other._buffer[i]);
		}
	}

	DynamicArray(DynamicArray &&other) :
			_capacity(other._capacity), _size(other._size) {
		_buffer = other._buffer;
		other._buffer = nullptr;
	}

	~DynamicArray() {
		release();
	}

	DynamicArray &operator=(const DynamicArray &other) {
		release();
		checkBufferSize(other._size);
		_size = other._size;
		for (size_t i = 0u; i < _size; ++i) {
			new (&_buffer[i]) TYPE(other._buffer[i]);
		}
		return *this;
	}

	DynamicArray &operator=(DynamicArray &&other) {
		release();
		_capacity = other._capacity;
		_size = other._size;
		_buffer = other._buffer;
		other._buffer = nullptr;
		return *this;
	}

	inline bool empty() const {
		return _size == 0u;
	}

	class iterator {
	private:
		TYPE* _ptr;
	public:
		constexpr iterator() :
			_ptr(nullptr) {
		}

		iterator(TYPE *ptr) :
			_ptr(ptr) {
		}

		inline const TYPE& operator*() const {
			return *_ptr;
		}

		inline TYPE& operator*() {
			return *_ptr;
		}

		iterator& operator++() {
			++_ptr;
			return *this;
		}

		iterator& operator+(size_t n) {
			_ptr += n;
			return *this;
		}

		iterator& operator+=(size_t n) {
			_ptr += n;
			return *this;
		}

		iterator& operator--() {
			--_ptr;
			return *this;
		}

		iterator& operator-(size_t n) {
			_ptr -= n;
			return *this;
		}

		iterator& operator-=(size_t n) {
			_ptr -= n;
			return *this;
		}

		inline const TYPE* operator->() const {
			return _ptr;
		}

		inline TYPE* operator->() {
			return _ptr;
		}

		inline bool operator!=(const iterator& rhs) const {
			return _ptr != rhs._ptr;
		}

		inline bool operator==(const iterator& rhs) const {
			return _ptr == rhs._ptr;
		}
	};

	template<typename... _Args>
	void emplace_back(_Args&&... args) {
		checkBufferSize(_size + 1u);
		new ((void *)&_buffer[_size++]) TYPE(core::forward<_Args>(args)...);
	}

	void push_back(const TYPE& val) {
		checkBufferSize(_size + 1u);
		new ((void *)&_buffer[_size++]) TYPE(val);
	}

	void pop() {
		core_assert(_size > 0u);
		_buffer[--_size].~TYPE();
	}

	TYPE* data() {
		return _buffer;
	}

	const TYPE* data() const {
		return _buffer;
	}

	TYPE& front() {
		core_assert(_size > 0u);
		return _buffer[0];
	}

	const TYPE& front() const {
		core_assert(_size > 0u);
		return _buffer[0];
	}

	TYPE& back() {
		core_assert(_size > 0u);
		return _buffer[_size - 1u];
	}

	const TYPE& back() const {
		core_assert(_size > 0u);
		return _buffer[_size - 1u];
	}

	void reserve(size_t size) {
		checkBufferSize(size);
	}

	void resize(size_t size) {
		checkBufferSize(size);
		while (size > _size) {
			push_back(TYPE());
		}
		while (size < _size) {
			pop();
		}
	}

	void clear() {
		for (size_t i = 0u; i < _size; ++i) {
			_buffer[i].~TYPE();
		}
		_size = 0u;
	}

	void release() {
		for (size_t i = 0u; i < _size; ++i) {
			_buffer[i].~TYPE();
		}
		core_free(_buffer);
		_capacity = 0u;
		_size = 0u;
		_buffer = nullptr;
	}

	void erase(size_t index, size_t n) {
		if (n == 0) {
			return;
		}
		if (index >= _size) {
			return;
		}
		const size_t delta = core_min(_size - index, n);
		const size_t srcIdxStart = index + delta;
		const size_t tgtIdxStart = index;
		size_t newSize = _size;
		if (srcIdxStart < _size) {
			size_t s = srcIdxStart;
			size_t t = tgtIdxStart;
			for (; s < _size; ++s, ++t) {
				_buffer[t].~TYPE();
				new ((void*)&_buffer[t]) TYPE(_buffer[s]);
			}
		}
		newSize -= delta;
		for (size_t i = newSize; i < _size; ++i) {
			_buffer[i].~TYPE();
		}
		_size = newSize;
	}

	inline size_t size() const {
		return _size;
	}

	inline size_t capacity() const {
		return _capacity;
	}

	inline iterator begin() const {
		return iterator(_buffer);
	}

	inline iterator end() const {
		return iterator(_buffer + _size);
	}

	inline const TYPE& operator[](size_t idx) const {
		core_assert(idx < _size);
		return _buffer[idx];
	}

	inline TYPE& operator[](size_t idx) {
		core_assert(idx < _size);
		return _buffer[idx];
	}
};

}
