/**
 * @file
 */

#include "core/concurrent/ReadWriteLock.h"
#include <SDL3/SDL_mutex.h>

namespace core {

ReadWriteLock::ReadWriteLock() : _rwlock(SDL_CreateRWLock()) {
}

ReadWriteLock::~ReadWriteLock() {
	SDL_DestroyRWLock(_rwlock);
	_rwlock = nullptr;
}

void ReadWriteLock::lockRead() const {
	SDL_LockRWLockForReading(_rwlock);
}

void ReadWriteLock::unlockRead() const {
	SDL_UnlockRWLock(_rwlock);
}

bool ReadWriteLock::tryLockRead() const {
	return SDL_TryLockRWLockForReading(_rwlock);
}

void ReadWriteLock::lockWrite() const {
	SDL_LockRWLockForWriting(_rwlock);
}

void ReadWriteLock::unlockWrite() const {
	SDL_UnlockRWLock(_rwlock);
}

bool ReadWriteLock::tryLockWrite() const {
	return SDL_TryLockRWLockForWriting(_rwlock);
}

} // namespace core
