/**
 * @file
 */

#pragma once

struct SDL_mutex;

namespace core {

class Lock {
private:
	mutable SDL_mutex* _mutex;
public:
	Lock();
	~Lock();

	void lock() const;
	void unlock() const;
	bool try_lock() const;

	SDL_mutex* handle();
};

class ScopedLock {
private:
	const Lock& _lock;
public:
	inline ScopedLock(const Lock& lock) : _lock(lock) {
		_lock.lock();
	}
	inline ~ScopedLock() {
		_lock.unlock();
	}
};

}
