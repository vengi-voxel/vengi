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
 * @brief Dynamically growing continuous storage buffer similiar to @c DynamicBuffer but without ctor and dtor handling
 *
 * @note This array does not have an upper size limit. Each time the capacity is reached, it will
 * allocate new slots given by the @c INCREASE template parameter.
 *
 * @note Don't use this class for non primitives
 * @note Use a fixed size array to prevent memory allocations - where possible
 * @sa DynamicArray
 * @ingroup Collections
 */
template<class TYPE, size_t INCREASE = 32u>
class Buffer {
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
		size_t newCapacity = align(newSize);
		TYPE* newBuffer = (TYPE*)core_malloc(newCapacity * sizeof(TYPE));
		if (_buffer != nullptr) {
			core_memcpy(newBuffer, _buffer, _capacity * sizeof(TYPE));
			core_free(_buffer);
		}
		_buffer = newBuffer;
		_capacity = newCapacity;
	}
public:
	using value_type = TYPE;

	Buffer() {
	}

	explicit Buffer(size_t amount) {
		checkBufferSize(amount);
		core_memset(_buffer, 0, _capacity * sizeof(TYPE));
		_size = amount;
	}

	Buffer(const Buffer& other) {
		checkBufferSize(other._size);
		_size = other._size;
		core_memcpy(_buffer, other._buffer, _size * sizeof(TYPE));
	}

	Buffer(Buffer &&other) :
			_capacity(other._capacity), _size(other._size) {
		_buffer = other._buffer;
		other._buffer = nullptr;
	}

	~Buffer() {
		release();
	}

	Buffer &operator=(const Buffer &other) {
		release();
		checkBufferSize(other._size);
		_size = other._size;
		core_memcpy(_buffer, other._buffer, _size * sizeof(TYPE));
		return *this;
	}

	Buffer &operator=(Buffer &&other) {
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

	void push_back(const TYPE& val) {
		checkBufferSize(_size + 1u);
		_buffer[_size++] = val;
	}

	void pop() {
		core_assert(_size > 0u);
		--_size;
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

	void clear() {
		_size = 0u;
	}

	void release() {
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
		if (srcIdxStart < _size) {
			size_t s = srcIdxStart;
			size_t t = tgtIdxStart;
			for (; s < _size;) {
				_buffer[t++] = _buffer[s++];
			}
		}
		_size -= delta;
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
