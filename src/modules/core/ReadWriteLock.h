/**
 * @file
 */

#pragma once

#include "core/String.h"

struct SDL_mutex;

namespace core {

class ReadWriteLock {
private:
	const core::String _name;
	mutable SDL_mutex* _mutex;
public:
	ReadWriteLock(const core::String& name);
	~ReadWriteLock();

	void lockRead() const;

	void unlockRead() const;

	void lockWrite();

	void unlockWrite();
};

class ScopedReadLock {
private:
	const ReadWriteLock& _lock;
public:
	inline ScopedReadLock(const ReadWriteLock& lock) : _lock(lock) {
		_lock.lockRead();
	}
	inline ~ScopedReadLock() {
		_lock.unlockRead();
	}
};

class ScopedWriteLock {
private:
	ReadWriteLock& _lock;
public:
	inline ScopedWriteLock(ReadWriteLock& lock) : _lock(lock) {
		_lock.lockWrite();
	}
	inline ~ScopedWriteLock() {
		_lock.unlockWrite();
	}
};

}
