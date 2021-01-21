/**
 * @file
 */

#pragma once

#include "core/concurrent/Concurrency.h"

#ifdef TRACY_ENABLE
#include "core/tracy/Tracy.hpp"
#endif

struct SDL_mutex;

namespace core {

class core_thread_capability("mutex") Lock {
private:
	mutable SDL_mutex *_mutex;

public:
#ifdef TRACY_ENABLE
	const tracy::SourceLocationData _srcLoc;
	mutable tracy::LockableCtx _ctx;
	Lock(const tracy::SourceLocationData &srcloc);
	void Mark(const tracy::SourceLocationData *srcloc);
	void CustomName(const char *name, size_t size);
#else
	Lock();
#endif
	~Lock();

	Lock(const Lock &) = delete;
	Lock &operator=(const Lock &) = delete;

	void lock() const core_thread_acquire();
	void unlock() const core_thread_release();
	bool try_lock() const core_thread_try_acquire(true);

	SDL_mutex *handle();
};

template<class LOCK>
class core_thread_scoped_capability ScopedLock {
private:
	LOCK &_lock;

public:
	inline ScopedLock(const LOCK &lock) core_thread_acquire(lock) : _lock(const_cast<LOCK &>(lock)) {
		_lock.lock();
	}
	inline ~ScopedLock() core_thread_release() {
		_lock.unlock();
	}
};

}
