/**
 * @file
 */

#pragma once

#include "core/concurrent/Concurrency.h"

#ifdef TRACY_ENABLE
#include "core/tracy/public/tracy/Tracy.hpp"
#endif
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
struct SDL_Mutex;
using core_mutex = SDL_Mutex;
#else
struct SDL_mutex;
using core_mutex = SDL_mutex;
#endif

namespace core {

// use core_trace_mutex to define a mutex
class core_thread_capability("mutex") Lock {
private:
	mutable core_mutex *_mutex;

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

	core_mutex *handle();
};

#ifdef TRACY_ENABLE
template<class LOCK>
#endif
class core_thread_scoped_capability ScopedLock {
private:
#ifdef TRACY_ENABLE
	LOCK &_lock;
#else
	Lock &_lock;
#endif

public:
#ifdef TRACY_ENABLE
	inline ScopedLock(const LOCK &lock) core_thread_acquire(lock) : _lock(const_cast<LOCK &>(lock)) {
#else
	inline ScopedLock(const Lock &lock) core_thread_acquire(lock) : _lock(const_cast<Lock &>(lock)) {
#endif
		_lock.lock();
	}
	inline ~ScopedLock() core_thread_release() {
		_lock.unlock();
	}
};

}
