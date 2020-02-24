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

void Lock::lock() const {
	SDL_LockMutex(_mutex);
}

void Lock::unlock() const {
	SDL_UnlockMutex(_mutex);
}

}
