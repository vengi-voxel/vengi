/**
 * @file
 */

#pragma once

#ifdef DEBUG
#define RWLOCKDEBUG 2000l
#endif

#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include "Trace.h"
#include "Common.h"
#include <mutex>

namespace core {

class ReadWriteLock {
private:
	const std::string _name;
	mutable std::mutex _mutex;
public:
	ReadWriteLock(const std::string& name) :
			_name(name) {
	}

	inline void lockRead() const {
		_mutex.lock();
	}

	inline void unlockRead() const {
		_mutex.unlock();
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
	ScopedReadLock(const ReadWriteLock& lock) : _lock(lock) {
		_lock.lockRead();
	}
	~ScopedReadLock() {
		_lock.unlockRead();
	}
};

class ScopedWriteLock {
private:
	ReadWriteLock& _lock;
public:
	ScopedWriteLock(ReadWriteLock& lock) : _lock(lock) {
		_lock.lockWrite();
	}
	~ScopedWriteLock() {
		_lock.unlockWrite();
	}
};

}
