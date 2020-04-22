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

template<class LOCK>
class ScopedLock {
private:
	LOCK& _lock;
public:
	inline ScopedLock(const LOCK& lock) : _lock(const_cast<LOCK&>(lock)) {
		_lock.lock();
	}
	inline ~ScopedLock() {
		_lock.unlock();
	}
};

}
