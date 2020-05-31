/**
 * @file
 */

#pragma once

#ifdef TRACY_ENABLE
#include "core/tracy/Tracy.hpp"
#endif

struct SDL_mutex;

namespace core {

class Lock {
private:
	mutable SDL_mutex* _mutex;
public:
#ifdef TRACY_ENABLE
	const tracy::SourceLocationData _srcLoc;
	mutable tracy::LockableCtx _ctx;
	Lock(const tracy::SourceLocationData& srcloc);
	void Mark(const tracy::SourceLocationData *srcloc);
	void CustomName(const char *name, size_t size);
#else
	Lock();
#endif
	~Lock();

	Lock(const Lock &) = delete;
	Lock &operator=(const Lock &) = delete;

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
