/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include <stdint.h>
#include <initializer_list>

namespace core {

/**
 * @brief Dynamically growing continuous storage buffer similiar to @c DynamicArray but without ctor and dtor handling
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
	static_assert(core::is_trivially_copyable<TYPE>::value, "only trivially copyable types are allowed - use DynamicArray<TYPE>");
	static_assert(core::is_trivially_destructible<TYPE>::value, "only trivially destructible types are allowed - use DynamicArray<TYPE>");
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
			core_memcpy(newBuffer, _buffer, core_min(newSize, _size) * sizeof(TYPE));
			core_free(_buffer);
		}
		_buffer = newBuffer;
		_capacity = newCapacity;
	}
public:
	using value_type = TYPE;

	constexpr Buffer() {
	}

	explicit Buffer(size_t amount) {
		if (amount == 0u) {
			return;
		}
		checkBufferSize(amount);
		core_memset(_buffer, 0, _capacity * sizeof(TYPE));
		_size = amount;
	}

	Buffer(size_t amount, int value) {
		if (amount == 0u) {
			return;
		}
		checkBufferSize(amount);
		core_memset(_buffer, value, _capacity * sizeof(TYPE));
		_size = amount;
	}

	Buffer(std::initializer_list<TYPE> other) {
		reserve(other.size());
		insert(end(), other.begin(), other.end());
	}

	Buffer(const Buffer& other) {
		checkBufferSize(other._size);
		_size = other._size;
		if (other._buffer != nullptr && _buffer != nullptr) {
			core_memcpy(_buffer, other._buffer, _size * sizeof(TYPE));
		}
	}

	Buffer(Buffer &&other) noexcept :
			_capacity(other._capacity), _size(other._size) {
		_buffer = other._buffer;
		other._buffer = nullptr;
		other._size = 0u;
		other._capacity = 0u;
	}

	~Buffer() {
		release();
	}

	Buffer &operator=(const Buffer &other) {
		if (&other == this) {
			return *this;
		}
		release();
		checkBufferSize(other._size);
		_size = other._size;
		if (other._buffer != nullptr) {
			core_memcpy(_buffer, other._buffer, _size * sizeof(TYPE));
		}
		return *this;
	}

	Buffer &operator=(Buffer &&other) noexcept {
		release();
		_capacity = other._capacity;
		_size = other._size;
		_buffer = other._buffer;
		other._buffer = nullptr;
		other._size = 0u;
		other._capacity = 0u;
		return *this;
	}

	CORE_FORCE_INLINE bool empty() const {
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

		iterator operator++(int) {
			return iterator(_ptr++);
		}

		iterator operator--(int) {
			return iterator(_ptr--);
		}

		int operator-(iterator rhs) const {
			return (int)(intptr_t)(_ptr - rhs._ptr);
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
	using const_iterator = iterator;

	void push_front(const TYPE& val) {
		insert(begin(), &val, 1);
	}

	inline void push_back(const TYPE& val) {
		checkBufferSize(_size + 1u);
		_buffer[_size++] = val;
	}

	template<typename... _Args>
	void emplace_back(_Args&&... args) {
		checkBufferSize(_size + 1u);
		_buffer[_size++] = TYPE(core::forward<_Args>(args)...);
	}

	void pop() {
		core_assert(_size > 0u);
		--_size;
	}

	void fill(const TYPE& value) {
		for (size_t i = 0u; i < _size; ++i) {
			_buffer[i] = value;
		}
	}

	/**
	 * @brief Insertion sort
	 * @note stable
	 * @note @c COMPARATOR must return true on @code lhs > rhs @endcode to sort ascending
	 */
	template<typename COMPARATOR>
	void sort(COMPARATOR comp) {
		int i;
		for (i = 1; i < (int)_size; ++i) {
			TYPE key = core::move(_buffer[i]);
			int j = i - 1;

			while (j >= 0 && comp(_buffer[j], key)) {
				_buffer[j + 1] = core::move(_buffer[j]);
				--j;
			}
			_buffer[j + 1] = core::move(key);
		}
	}

	CORE_FORCE_INLINE TYPE* data() {
		return _buffer;
	}

	CORE_FORCE_INLINE const TYPE* data() const {
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

	void reset() {
		_size = 0u;
	}

	template<class COLLECTION>
	void append(const COLLECTION& collection) {
		const size_t n = collection.size();
		checkBufferSize(_size + n);
		for (size_t i = 0u; i < n; ++i) {
			_buffer[_size++] = collection[i];
		}
	}

	void append(const TYPE* array, size_t n) {
		checkBufferSize(_size + n);
		core_memcpy(&_buffer[_size], array, n * sizeof(TYPE));
		_size += n;
	}

	template<class FUNC>
	void append(size_t n, FUNC&& func) {
		checkBufferSize(_size + n);
		for (size_t i = 0u; i < n; ++i) {
			_buffer[_size++] = func(i);
		}
	}

	inline void insert(iterator pos, const TYPE& value) {
		insert(pos, &value, 1);
	}

	void insert(iterator pos, const TYPE* array, size_t n) {
		if (n == 0) {
			return;
		}
		if (pos == end()) {
			append(array, n);
			return;
		}

		const size_t startIdx = index(pos);
		size_t s = _size - 1;
		size_t t = s + n;

		checkBufferSize(_size + n);

		const size_t cnt = _size - startIdx;
		for (size_t i = 0; i < cnt; ++i, --s, --t) {
			_buffer[t] = core::move(_buffer[s]);
			if (s == 0) {
				break;
			}
		}

		for (size_t i = 0u; i < n; ++i) {
			_buffer[startIdx + i] = array[i];
		}
		_size += n;
	}

	/**
	 * @brief This might shrink the buffer capacity. If any new slots were added, they are not initialized
	 */
	void resize(size_t size) {
		checkBufferSize(size);
		for (size_t i = _size; i < size; ++i) {
			_buffer[i] = TYPE();
		}
		_size = size;
	}

	template<typename ITER>
	void insert(iterator pos, ITER first, ITER last) {
		if (first == last) {
			return;
		}

		const int n = last - first;

		if (pos == end()) {
			checkBufferSize(_size + n);

			for (ITER i = first; i != last; ++i) {
				_buffer[_size++] = *i;
			}
			return;
		}

		size_t startIdx = index(pos);

		checkBufferSize(_size + n);

		size_t s = _size - 1;
		size_t t = s + n;

		const size_t cnt = _size - startIdx;
		for (size_t i = 0; i < cnt; ++i, --s, --t) {
			_buffer[t] = core::move(_buffer[s]);
			if (s == 0) {
				break;
			}
		}

		for (ITER i = first; i != last; ++i) {
			_buffer[startIdx++] = *i;
		}
		_size += n;
	}

	void erase(size_t index, size_t n = 1u) {
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

	bool erase(iterator iter, size_t n = 1) {
		if (iter == end()) {
			return false;
		}
		erase(index(iter), n);
		return true;
	}

	CORE_FORCE_INLINE size_t size() const {
		return _size;
	}

	CORE_FORCE_INLINE size_t capacity() const {
		return _capacity;
	}

	CORE_FORCE_INLINE iterator begin() const {
		return iterator(_buffer);
	}

	CORE_FORCE_INLINE iterator end() const {
		return iterator(_buffer + _size);
	}

	CORE_FORCE_INLINE const TYPE& operator[](size_t idx) const {
		core_assert(idx < _size);
		return _buffer[idx];
	}

	CORE_FORCE_INLINE TYPE& operator[](size_t idx) {
		core_assert(idx < _size);
		return _buffer[idx];
	}

private:
	CORE_FORCE_INLINE constexpr size_t index(const_iterator iter) const {
		if (iter == begin()) {
			return 0;
		}
		const TYPE* ptr = iter.operator->();
		const size_t idx = ptr - begin().operator->();
		return idx;
	}
};

}
