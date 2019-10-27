/**
 * @file
 */

#pragma once

#include <string>
#include <thread>
#include <mutex>
#include "Trace.h"
#define CORE_SHARED_MUTEX 0
#define CORE_RECURSIVE_MUTEX 0

#if CORE_SHARED_MUTEX
#include <shared_mutex>
#if __cplusplus > 201402L
using Mutex = std::shared_mutex;
#else
using Mutex = std::shared_timed_mutex;
#endif
#else // CORE_SHARED_MUTEX
#if CORE_RECURSIVE_MUTEX
using Mutex = std::recursive_mutex;
#else // CORE_RECURSIVE_MUTEX
using Mutex = std::mutex;
#endif // CORE_RECURSIVE_MUTEX
#endif // CORE_SHARED_MUTEX
#include <atomic>
#include <chrono>

namespace core {

class ReadWriteLock {
private:
	const std::string _name;
	mutable core_trace_mutex(Mutex, _mutex);
public:
	ReadWriteLock(const std::string& name) :
			_name(name) {
	}

	inline void lockRead() const {
#if CORE_SHARED_MUTEX
		_mutex.lock_shared();
#else
		_mutex.lock();
#endif
	}

	inline void unlockRead() const {
#if CORE_SHARED_MUTEX
		_mutex.unlock_shared();
#else
		_mutex.unlock();
#endif
	}

	inline void lockWrite() {
		_mutex.lock();
	}

	inline void unlockWrite() {
		_mutex.unlock();
	}
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
