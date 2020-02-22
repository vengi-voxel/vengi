/**
 * @file
 */

#include "Atomic.h"

namespace core {

AtomicBool::AtomicBool(bool value) {
	SDL_AtomicSet(&_value, (int)value);
}

AtomicBool::operator bool() const {
	return SDL_AtomicGet(const_cast<SDL_atomic_t*>(&_value)) == 1;
}

bool AtomicBool::exchange(bool rhs) {
	return SDL_AtomicSet(&_value, rhs) == 1;
}

void AtomicBool::operator=(bool rhs) {
	SDL_AtomicSet(&_value, (int)rhs);
}

void AtomicBool::operator=(const AtomicBool& rhs) {
	SDL_AtomicSet(&_value, SDL_AtomicGet(const_cast<SDL_atomic_t*>(&rhs._value)));
}

bool AtomicBool::operator==(bool rhs) const {
	return (bool)(*this) == rhs;
}

bool AtomicBool::operator==(const AtomicBool& rhs) const {
	return (bool)(*this) == (bool)rhs;
}

AtomicInt::AtomicInt(int value) {
	SDL_AtomicSet(&_value, value);
}

AtomicInt::operator int() const {
	return SDL_AtomicGet(const_cast<SDL_atomic_t*>(&_value));
}

int AtomicInt::exchange(int rhs) {
	return SDL_AtomicSet(&_value, rhs);
}

void AtomicInt::operator=(int rhs) {
	SDL_AtomicSet(&_value, rhs);
}

void AtomicInt::operator=(const AtomicInt& rhs) {
	SDL_AtomicSet(&_value, SDL_AtomicGet(const_cast<SDL_atomic_t*>(&rhs._value)));
}

bool AtomicInt::operator==(int rhs) const {
	return (int)(*this) == rhs;
}

bool AtomicInt::operator==(const AtomicInt& rhs) const {
	return (int)(*this) == (int)rhs;
}

AtomicInt& AtomicInt::operator--() {
	SDL_AtomicAdd(&_value, -1);
	return *this;
}

AtomicInt& AtomicInt::operator++() {
	SDL_AtomicAdd(&_value, 1);
	return *this;
}

int AtomicInt::decrement(int value) {
	return SDL_AtomicAdd(&_value, -value);
}

int AtomicInt::increment(int value) {
	return SDL_AtomicAdd(&_value, value);
}

}
