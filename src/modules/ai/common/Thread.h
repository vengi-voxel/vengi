#pragma once

#include "Types.h"
#include <thread>
#include <mutex>
#include <atomic>
#if AI_DEBUG_READWRITELOCK > 0
#include <chrono>
#endif

namespace ai {

class ReadWriteLock {
private:
	mutable std::atomic_int _readers;
	mutable std::atomic_bool _lock;
	const std::string _name;
	std::thread::id _threadID;
	int _recursive;
public:
	ReadWriteLock(const std::string& name) : _readers(0), _lock(false), _name(name), _recursive(0) {}

	inline void lockRead() const {
#if AI_DEBUG_READWRITELOCK > 0
		auto start = std::chrono::system_clock::now();
#endif
		while (_lock) {
			std::this_thread::yield();
#if AI_DEBUG_READWRITELOCK > 0
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> diff = end - start;
			ai_assert(diff.count() < AI_DEBUG_READWRITELOCK, "%s is blocked longer than %ims", _name.c_str(), AI_DEBUG_READWRITELOCK);
#endif
		}
		++_readers;
	}

	inline void unlockRead() const {
		--_readers;
	}

	inline void lockWrite() {
		std::thread::id threadId = std::this_thread::get_id();
		if (_threadID == threadId) {
			++_recursive;
			return;
		}

#if AI_DEBUG_READWRITELOCK > 0
		auto start = std::chrono::system_clock::now();
#endif
		while (std::atomic_exchange_explicit(&_lock, true, std::memory_order_acquire)) {
			std::this_thread::yield();
#if AI_DEBUG_READWRITELOCK > 0
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> diff = end - start;
			ai_assert(diff.count() < AI_DEBUG_READWRITELOCK, "%s is blocked longer than %ims", _name.c_str(), AI_DEBUG_READWRITELOCK);
#endif
		}
		while (_readers > 0) {
			std::this_thread::yield();
#if AI_DEBUG_READWRITELOCK > 0
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> diff = end - start;
			ai_assert(diff.count() < AI_DEBUG_READWRITELOCK, "%s is blocked longer than %ims", _name.c_str(), AI_DEBUG_READWRITELOCK);
#endif
		}
		ai_assert(_recursive == 0, "Invalid state for unlocking a write lock of %s", _name.c_str());
		_threadID = threadId;
		++_recursive;
	}

	inline void unlockWrite() {
		ai_assert(_threadID == std::this_thread::get_id(), "Invalid thread locked %s", _name.c_str());
		ai_assert(_recursive > 0, "Invalid state for unlocking a write lock of %s", _name.c_str());
		--_recursive;
		if (_recursive == 0) {
			_threadID = std::thread::id();
			std::atomic_store_explicit(&_lock, false, std::memory_order_release);
		}
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
