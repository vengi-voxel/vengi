/**
 * @file
 */

#pragma once

#include <SDL_atomic.h>

namespace core {

class AtomicBool {
private:
	SDL_atomic_t _value { 0 };
public:
	AtomicBool(bool value);

	operator bool() const;

	bool exchange(bool rhs);

	void operator=(bool rhs);
	void operator=(const AtomicBool& rhs);

	bool operator==(bool rhs) const;
	bool operator==(const AtomicBool& rhs) const;
};

class AtomicInt {
private:
	SDL_atomic_t _value { 0 };
public:
	AtomicInt(int value);

	operator int() const;

	int exchange(int rhs);

	void operator=(int rhs);
	void operator=(const AtomicInt& rhs);

	AtomicInt& operator--();
	AtomicInt& operator++();

	bool operator==(int rhs) const;
	bool operator==(const AtomicInt& rhs) const;
};

}
