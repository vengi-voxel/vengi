/**
 * @file
 */

#pragma once

#include "core/Algorithm.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include <cstdint> // intptr_t - not available in stdint.h
#include <new>
#include <initializer_list>
#include <SDL_version.h>


#if SDL_VERSION_ATLEAST(3, 2, 0)

extern "C" size_t SDLCALL SDL_GetSIMDAlignment();
extern "C" void * SDLCALL SDL_aligned_alloc(size_t alignment, size_t size);

#ifndef core_aligned_malloc
#define core_aligned_malloc(size) SDL_aligned_alloc(SDL_GetSIMDAlignment(), size)
#endif

#ifndef core_aligned_free
#define core_aligned_free SDL_aligned_free
#endif

#else

extern "C" void SDLCALL SDL_SIMDFree(void *ptr);
extern "C" void *SDLCALL SDL_SIMDAlloc(const size_t len);

#ifndef core_aligned_malloc
#define core_aligned_malloc SDL_SIMDAlloc
#endif

#ifndef core_aligned_free
#define core_aligned_free SDL_SIMDFree
#endif
#endif

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
	// static_assert(!core::is_trivially_copyable<TYPE>::value, "no trivial types are allowed - use Buffer<TYPE>");
	TYPE* _buffer = nullptr;
	size_t _capacity = 0u;
	size_t _size = 0u;
	size_t _increase = INCREASE;

	inline constexpr size_t align(size_t val) {
		const size_t len = _increase - 1u;
		return (size_t)((val + len) & ~len);
	}

	void checkBufferSize(size_t newSize) {
		if (_capacity >= newSize) {
			return;
		}
		_capacity = align(newSize);
		TYPE* newBuffer = (TYPE*)core_aligned_malloc(_capacity * sizeof(TYPE));
		for (size_t i = 0u; i < _size; ++i) {
			new ((void*)&newBuffer[i]) TYPE(core::move(_buffer[i]));
			_buffer[i].~TYPE();
		}
		core_aligned_free(_buffer);
		_buffer = newBuffer;
	}
public:
	using value_type = TYPE;

	DynamicArray() {
	}

	DynamicArray(std::initializer_list<TYPE> other) {
		insert(end(), other.begin(), other.end());
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

	DynamicArray(DynamicArray &&other) noexcept :
			_capacity(other._capacity), _size(other._size) {
		_buffer = other._buffer;
		other._buffer = nullptr;
		other._size = 0;
		other._capacity = 0;
	}

	~DynamicArray() {
		release();
	}

	void setIncrease(size_t increase) {
		_increase = increase;
	}

	size_t increase() const {
		return _increase;
	}

	DynamicArray &operator=(const DynamicArray &other) {
		if (&other == this) {
			return *this;
		}
		release();
		checkBufferSize(other._size);
		_size = other._size;
		for (size_t i = 0u; i < _size; ++i) {
			new (&_buffer[i]) TYPE(other._buffer[i]);
		}
		return *this;
	}

	DynamicArray &operator=(DynamicArray &&other) noexcept {
		release();
		_capacity = other._capacity;
		_size = other._size;
		_buffer = other._buffer;
		other._buffer = nullptr;
		other._size = 0;
		other._capacity = 0;
		return *this;
	}

	inline bool empty() const {
		return _size == 0u;
	}

	inline size_t bytes() const {
		return _size * sizeof(TYPE);
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

	template<typename... _Args>
	void emplace_back(_Args&&... args) {
		checkBufferSize(_size + 1u);
		new ((void *)&_buffer[_size++]) TYPE(core::forward<_Args>(args)...);
	}

	void push_front(const TYPE& val) {
		insert(begin(), val);
	}

	void push_back(const TYPE& val) {
		checkBufferSize(_size + 1u);
		new ((void *)&_buffer[_size++]) TYPE(val);
	}

	template<class COLLECTION>
	void append(const COLLECTION& collection) {
		const size_t n = collection.size();
		checkBufferSize(_size + n);
		for (size_t i = 0u; i < n; ++i) {
			new ((void *)&_buffer[_size++]) TYPE(collection[i]);
		}
	}

	template<class COLLECTION>
	void append(COLLECTION&& collection) {
		const size_t n = collection.size();
		checkBufferSize(_size + n);
		for (size_t i = 0u; i < n; ++i) {
			new ((void *)&_buffer[_size++]) TYPE(core::move(collection[i]));
		}
	}

	void append(const TYPE* array, size_t n) {
		checkBufferSize(_size + n);
		for (size_t i = 0u; i < n; ++i) {
			new ((void *)&_buffer[_size++]) TYPE(array[i]);
		}
	}

	template<class FUNC>
	void append(size_t n, FUNC&& func) {
		checkBufferSize(_size + n);
		for (size_t i = 0u; i < n; ++i) {
			new ((void *)&_buffer[_size++]) TYPE(func(i));
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

		// TODO: this can be optimized by only calling the move ctor once
		checkBufferSize(_size + n);

		const size_t cnt = _size - startIdx;
		for (size_t i = 0; i < cnt; ++i, --s, --t) {
			new ((void*)&_buffer[t]) TYPE(core::move(_buffer[s]));
			_buffer[s].~TYPE();
			if (s == 0) {
				break;
			}
		}

		for (size_t i = 0u; i < n; ++i) {
			new ((void *)&_buffer[startIdx + i]) TYPE(array[i]);
		}
		_size += n;
	}

	template<typename ITER>
	void insert(iterator pos, ITER first, ITER last) {
		if (first == last) {
			return;
		}

		const int n = core::distance(first, last);

		if (pos == end()) {
			checkBufferSize(_size + n);

			for (ITER i = first; i != last; ++i) {
				new ((void *)&_buffer[_size++]) TYPE(*i);
			}
			return;
		}

		size_t startIdx = index(pos);

		// TODO: this can be optimized by only calling the move ctor once
		checkBufferSize(_size + n);

		size_t s = _size - 1;
		size_t t = s + n;

		const size_t cnt = _size - startIdx;
		for (size_t i = 0; i < cnt; ++i, --s, --t) {
			new ((void*)&_buffer[t]) TYPE(core::move(_buffer[s]));
			_buffer[s].~TYPE();
			if (s == 0) {
				break;
			}
		}

		for (ITER i = first; i != last; ++i) {
			new ((void *)&_buffer[startIdx++]) TYPE(*i);
		}
		_size += n;
	}

	void pop() {
		core_assert(_size > 0u);
		if (_size > 0u) {
			_buffer[--_size].~TYPE();
		}
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

	void insert(size_t size, const TYPE& type) {
		checkBufferSize(size);
		while (size > _size) {
			push_back(type);
		}
		while (size < _size) {
			pop();
		}
	}

	void resize(size_t size) {
		checkBufferSize(size);
		while (size > _size) {
			emplace_back(TYPE{});
		}
		while (size < _size) {
			pop();
		}
	}

	void fill(const TYPE& value) {
		for (size_t i = 0u; i < _size; ++i) {
			_buffer[i].~TYPE();
			new (&_buffer[i]) TYPE(value);
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
		core_aligned_free(_buffer);
		_capacity = 0u;
		_size = 0u;
		_buffer = nullptr;
	}

	bool erase(iterator iter, size_t n = 1) {
		if (iter == end()) {
			return false;
		}
		erase(index(iter), n);
		return true;
	}

	void erase(size_t index, size_t n = 1) {
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
				new ((void*)&_buffer[t]) TYPE(core::move(_buffer[s]));
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
		core_assert_msg(idx < _size, "idx is out of bounds: %i vs %i", (int)idx, (int)_size);
		return _buffer[idx];
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

	inline TYPE& operator[](size_t idx) {
		core_assert_msg(idx < _size, "idx is out of bounds: %i vs %i", (int)idx, (int)_size);
		return _buffer[idx];
	}

private:
	constexpr size_t index(const_iterator iter) const {
		if (iter == begin()) {
			return 0;
		}
		const TYPE* ptr = iter.operator->();
		const size_t idx = ptr - begin().operator->();
		return idx;
	}
};

}
