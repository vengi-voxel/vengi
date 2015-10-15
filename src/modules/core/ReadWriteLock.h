#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

namespace core {

class ReadWriteLock {
private:
	mutable std::atomic_int _readers;
	mutable std::atomic<bool> _lock;
	const std::string _name;
public:
	ReadWriteLock(const std::string& name) : _readers(0), _lock(false), _name(name) {}

	inline void lockRead() const {
		while (_lock) {}
		++_readers;
	}

	inline void unlockRead() const {
		--_readers;
	}

	inline void lockWrite() {
		while (std::atomic_exchange_explicit(&_lock, true, std::memory_order_acquire)) {}
		while (_readers > 0) {}
	}

	inline void unlockWrite() {
		std::atomic_store_explicit(&_lock, false, std::memory_order_release);
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
