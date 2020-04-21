/**
 * @file
 */

#include "Lock.h"
#include <SDL_mutex.h>

namespace core {

Lock::Lock() :
		_mutex(SDL_CreateMutex()) {
}

Lock::~Lock() {
	SDL_DestroyMutex(_mutex);
}

SDL_mutex* Lock::handle() {
	return _mutex;
}

void Lock::lock() const {
	SDL_LockMutex(_mutex);
}

void Lock::unlock() const {
	SDL_UnlockMutex(_mutex);
}

bool Lock::try_lock() const {
	return SDL_TryLockMutex(_mutex) == 0;
}

}
