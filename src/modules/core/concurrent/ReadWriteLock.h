/**
 * @file
 */

#pragma once

#include "core/concurrent/Concurrency.h"

struct SDL_RWLock;

namespace core {

/**
 * @brief Read/write lock (multiple readers or one writer).
 *
 * SDL types are opaque here; the implementation lives in the .cpp so
 * call sites do not include SDL headers.
 */
class core_thread_capability("mutex") ReadWriteLock {
private:
	mutable SDL_RWLock *_rwlock;

public:
	ReadWriteLock();
	~ReadWriteLock();

	ReadWriteLock(const ReadWriteLock &) = delete;
	ReadWriteLock &operator=(const ReadWriteLock &) = delete;

	void lockRead() const core_thread_acquire_shared();
	void unlockRead() const core_thread_release_shared();
	bool tryLockRead() const core_thread_try_acquire_shared(true);

	void lockWrite() const core_thread_acquire();
	void unlockWrite() const core_thread_release();
	bool tryLockWrite() const core_thread_try_acquire(true);
};

class core_thread_scoped_capability ScopedReadLock {
private:
	const ReadWriteLock &_lock;

public:
	inline explicit ScopedReadLock(const ReadWriteLock &lock) core_thread_acquire_shared(lock) : _lock(lock) {
		_lock.lockRead();
	}
	inline ~ScopedReadLock() core_thread_release_shared() {
		_lock.unlockRead();
	}

	ScopedReadLock(const ScopedReadLock &) = delete;
	ScopedReadLock &operator=(const ScopedReadLock &) = delete;
};

class core_thread_scoped_capability ScopedWriteLock {
private:
	const ReadWriteLock &_lock;

public:
	inline explicit ScopedWriteLock(const ReadWriteLock &lock) core_thread_acquire(lock) : _lock(lock) {
		_lock.lockWrite();
	}
	inline ~ScopedWriteLock() core_thread_release() {
		_lock.unlockWrite();
	}

	ScopedWriteLock(const ScopedWriteLock &) = delete;
	ScopedWriteLock &operator=(const ScopedWriteLock &) = delete;
};

} // namespace core
