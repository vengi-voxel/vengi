/**
 * @file
 */

#pragma once

#include <SDL_atomic.h>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
struct SDL_Condition;
using core_atomic = SDL_AtomicInt;
#else
struct SDL_cond;
using core_atomic = SDL_atomic_t;
#endif

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


#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDL_AtomicSetPtr SDL_SetAtomicPointer
#define SDL_AtomicGetPtr SDL_GetAtomicPointer
#define SDL_AtomicCASPtr SDL_CompareAndSwapAtomicPointer
#endif

template<class T>
class AtomicPtr {
private:
	void* _ptr;
public:
	AtomicPtr(T* value = nullptr) {
		SDL_AtomicSetPtr(&_ptr, (void*)value);
	}

	operator T*() {
		return (T*)SDL_AtomicGetPtr(&_ptr);
	}

	operator const T*() const {
		return (const T*)SDL_AtomicGetPtr(const_cast<void**>(&_ptr));
	}

	T* exchange(T* value) {
		return (T*)SDL_AtomicSetPtr(&_ptr, (void*)value);
	}

	T* compare_exchange(T* expectedPtr, T* newPtr) {
		return (T*)SDL_AtomicCASPtr(&_ptr, (void*)expectedPtr, (void*)newPtr);
	}

	void operator=(T* value) {
		SDL_AtomicSetPtr(&_ptr, (void*)value);
	}

	void operator=(const AtomicPtr& value) {
		if (&value == this) {
			return;
		}
		SDL_AtomicSetPtr(&_ptr, value._ptr);
	}

	bool operator==(T* value) const {
		return (const T*)(*this) == value;
	}

	bool operator==(const AtomicPtr& value) const {
		return (const T*)(*this) == (const T*)value;
	}
};

}
