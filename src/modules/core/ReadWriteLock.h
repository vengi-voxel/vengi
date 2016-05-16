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

#define USE_MUTEX 1

#if USE_MUTEX != 0
class ReadWriteLock {
private:
	const std::string _name;
	mutable std::recursive_mutex _mutex;
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
#else
class ReadWriteLock {
private:
	mutable std::atomic_int _readers;
	mutable std::atomic_bool _lock;
	const std::string _name;
	std::atomic<std::thread::id> _threadID;
	std::atomic_int _recursive;
public:
	ReadWriteLock(const std::string& name) :
			_readers(0), _lock(false), _name(name), _recursive(0) {
	}

	inline void lockRead() const {
		core_trace_scoped(LockRead);
		if (_threadID == std::this_thread::get_id()) {
			++_readers;
			return;
		}
#if RWLOCKDEBUG > 0
		auto start = std::chrono::system_clock::now();
#endif
		while (_lock) {
			std::this_thread::yield();
#if RWLOCKDEBUG > 0
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> diff = end - start;
			core_assert_msg(diff.count() < RWLOCKDEBUG, "%s is blocked longer than %lims", _name.c_str(), RWLOCKDEBUG);
#endif
		}
		++_readers;
	}

	inline void unlockRead() const {
		--_readers;
	}

	inline void lockWrite() {
		core_trace_scoped(LockWrite);
		std::thread::id threadId = std::this_thread::get_id();
		if (_threadID == threadId) {
			++_recursive;
			return;
		}

#if RWLOCKDEBUG > 0
		auto start = std::chrono::system_clock::now();
#endif
		while (std::atomic_exchange_explicit(&_lock, true, std::memory_order_acquire)) {
			std::this_thread::yield();
#if RWLOCKDEBUG > 0
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> diff = end - start;
			core_assert_msg(diff.count() < RWLOCKDEBUG, "%s is blocked longer than %lims", _name.c_str(), RWLOCKDEBUG);
#endif
		}
		while (_readers > 0) {
			std::this_thread::yield();
#if RWLOCKDEBUG > 0
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> diff = end - start;
			core_assert_msg(diff.count() < RWLOCKDEBUG, "%s is blocked longer than %lims", _name.c_str(), RWLOCKDEBUG);
#endif
		}
		core_assert_msg(_recursive == 0, "Invalid state for unlocking a write lock of %s", _name.c_str());
		_threadID = threadId;
		++_recursive;
	}

	inline void unlockWrite() {
		core_assert_msg(_threadID == std::this_thread::get_id(), "Invalid thread locked %s", _name.c_str());
		core_assert_msg(_recursive > 0, "Invalid state for unlocking a write lock of %s", _name.c_str());
		--_recursive;
		if (_recursive == 0) {
			_threadID = std::thread::id();
			std::atomic_store_explicit(&_lock, false, std::memory_order_release);
		}
	}
};
#endif

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
