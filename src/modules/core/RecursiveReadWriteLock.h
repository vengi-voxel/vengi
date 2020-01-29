/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <mutex>

namespace core {

class RecursiveReadWriteLock {
private:
	const std::string _name;
	mutable std::recursive_mutex _mutex;
public:
	RecursiveReadWriteLock(const std::string& name) :
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

class RecursiveScopedReadLock {
private:
	const RecursiveReadWriteLock& _lock;
public:
	inline RecursiveScopedReadLock(const RecursiveReadWriteLock& lock) : _lock(lock) {
		_lock.lockRead();
	}
	inline ~RecursiveScopedReadLock() {
		_lock.unlockRead();
	}
};

class RecursiveScopedWriteLock {
private:
	RecursiveReadWriteLock& _lock;
public:
	inline RecursiveScopedWriteLock(RecursiveReadWriteLock& lock) : _lock(lock) {
		_lock.lockWrite();
	}
	inline ~RecursiveScopedWriteLock() {
		_lock.unlockWrite();
	}
};

}
