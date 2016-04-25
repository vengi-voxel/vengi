#pragma once

#ifdef DEBUG
#define RWLOCKDEBUG
#define LOCK_DURATION 2000l
#endif

#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include "Trace.h"
#ifdef RWLOCKDEBUG
#include <chrono>
#include "core/Common.h"
#endif

namespace core {

class ReadWriteLock {
private:
	mutable std::atomic_int _readers;
	mutable std::atomic<bool> _lock;
	const std::string _name;
public:
	ReadWriteLock(const std::string& name) : _readers(0), _lock(false), _name(name) {}

	inline void lockRead() const {
		core_trace_scoped(LockRead);
#ifdef RWLOCKDEBUG
		auto start = std::chrono::system_clock::now();
#endif
		while (_lock) {
			std::this_thread::yield();
#ifdef RWLOCKDEBUG
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<long> diff = end-start;
			core_assert_msg(diff.count() < LOCK_DURATION, "%s is blocked longer than %lims", _name.c_str(), LOCK_DURATION);
#endif
		}
		++_readers;
	}

	inline void unlockRead() const {
		--_readers;
	}

	inline void lockWrite() {
		core_trace_scoped(LockWrite);
#ifdef RWLOCKDEBUG
		auto start = std::chrono::system_clock::now();
#endif
		while (std::atomic_exchange_explicit(&_lock, true, std::memory_order_acquire)) {
			std::this_thread::yield();
#ifdef RWLOCKDEBUG
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<long> diff = end-start;
			core_assert_msg(diff.count() < LOCK_DURATION, "%s is blocked longer than %lims", _name.c_str(), LOCK_DURATION);
#endif
		}
		while (_readers > 0) {
			std::this_thread::yield();
#ifdef RWLOCKDEBUG
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<long> diff = end-start;
			core_assert_msg(diff.count() < LOCK_DURATION, "%s is blocked longer than %lims", _name.c_str(), LOCK_DURATION);
#endif
		}
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
