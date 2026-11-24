/**
 * @file
 */

#pragma once

#include "core/concurrent/Concurrency.h"

#ifdef TRACY_ENABLE
namespace tracy {
struct SourceLocationData;
class LockableCtx;
} // namespace tracy
#endif

struct SDL_Mutex;
using core_mutex = SDL_Mutex;

namespace core {

#ifdef TRACY_ENABLE
/**
 * One immortal source location per callsite. Kept off the Tracy.hpp include path so
 * Lock users do not parse the profiler headers.
 */
const tracy::SourceLocationData *traceLockLocation(const char *name, const char *file, uint32_t line);
#endif

// use core_trace_mutex to define a mutex
class core_thread_capability("mutex") Lock {
private:
	mutable core_mutex *_mutex;
#ifdef TRACY_ENABLE
	// Pointer so this header does not include Tracy.hpp (LockableCtx pulls the profiler).
	mutable tracy::LockableCtx *_ctx;
#endif

public:
#ifdef TRACY_ENABLE
	Lock(const tracy::SourceLocationData *srcloc);
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

} // namespace core

#ifdef TRACY_ENABLE
#define core_trace_mutex(type, varname, name)                                                                          \
	type varname {                                                                                                     \
		[]() -> const tracy::SourceLocationData * {                                                                    \
			static const tracy::SourceLocationData *srcloc =                                                           \
				core::traceLockLocation(name, __FILE__, (uint32_t)__LINE__);                                           \
			return srcloc;                                                                                             \
		}()                                                                                                            \
	}
#define core_trace_mutex_static(type, classname, name)                                                                 \
	type classname::name {                                                                                             \
		[]() -> const tracy::SourceLocationData * {                                                                    \
			static const tracy::SourceLocationData *srcloc =                                                           \
				core::traceLockLocation(#name, __FILE__, (uint32_t)__LINE__);                                          \
			return srcloc;                                                                                             \
		}()                                                                                                            \
	}
#else
#define core_trace_mutex(type, varname, name) type varname
#define core_trace_mutex_static(type, classname, name) type classname::name
#endif
