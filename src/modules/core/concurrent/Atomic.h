/**
 * @file
 */

#pragma once

#include <SDL3/SDL_atomic.h>

struct SDL_Condition;
using core_atomic = SDL_AtomicInt;

namespace core {

class AtomicBool {
private:
	core_atomic _value { 0 };
public:
	AtomicBool(bool value);
	AtomicBool(const AtomicBool& rhs);

	operator bool() const;

	bool exchange(bool rhs);
	bool compare_exchange(bool expectedVal, bool newVal);

	void operator=(bool rhs);
	void operator=(const AtomicBool& rhs);

	bool operator==(bool rhs) const;
	bool operator==(const AtomicBool& rhs) const;
};

class AtomicInt {
private:
	core_atomic _value;
public:
	AtomicInt(int value = 0);

	operator int() const;

	int exchange(int rhs);
	bool compare_exchange(int expectedVal, int newVal);

	void operator=(int rhs);
	void operator=(const AtomicInt& rhs);

	int decrement(int value = 1);
	int increment(int value = 1);

	AtomicInt& operator--();
	AtomicInt& operator++();

	bool operator==(int rhs) const;
	bool operator==(const AtomicInt& rhs) const;
};

template<class T>
class AtomicPtr {
private:
	void* _ptr;
public:
	AtomicPtr(T* value = nullptr) {
		SDL_SetAtomicPointer(&_ptr, (void*)value);
	}

	AtomicPtr(const AtomicPtr& value) : AtomicPtr(value.load()) {
	}

	/** Atomically load the pointer, including from a const atomic. */
	T* load() const {
		return (T*)SDL_GetAtomicPointer(const_cast<void**>(&_ptr));
	}

	operator T*() {
		return load();
	}

	operator const T*() const {
		return load();
	}

	T* exchange(T* value) {
		return (T*)SDL_SetAtomicPointer(&_ptr, (void*)value);
	}

	bool compare_exchange(T* expectedPtr, T* newPtr) {
		return SDL_CompareAndSwapAtomicPointer(&_ptr, (void*)expectedPtr, (void*)newPtr);
	}

	void operator=(T* value) {
		SDL_SetAtomicPointer(&_ptr, (void*)value);
	}

	void operator=(const AtomicPtr& value) {
		if (&value == this) {
			return;
		}
		SDL_SetAtomicPointer(&_ptr, value.load());
	}

	bool operator==(T* value) const {
		return (const T*)(*this) == value;
	}

	bool operator==(const AtomicPtr& value) const {
		return (const T*)(*this) == (const T*)value;
	}
};

}
