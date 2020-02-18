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

}