/**
 * @file
 */

#include "Lock.h"
#include <SDL3/SDL_mutex.h>

#ifdef TRACY_ENABLE
#include "core/tracy/public/tracy/Tracy.hpp"
#endif

namespace core {

#ifdef TRACY_ENABLE
const tracy::SourceLocationData *traceLockLocation(const char *name, const char *file, uint32_t line) {
	// Immortal, one per callsite (the macro caches the pointer). Same lifetime as the old
	// function-local static SourceLocationData.
	return new tracy::SourceLocationData{name, name, file, line, 0};
}

Lock::Lock(const tracy::SourceLocationData *srcloc) : _mutex(SDL_CreateMutex()), _ctx(new tracy::LockableCtx(srcloc)) {
}

void Lock::Mark(const tracy::SourceLocationData *srcloc) {
	_ctx->Mark(srcloc);
}

void Lock::CustomName(const char *name, size_t size) {
	_ctx->CustomName(name, size);
}
#else
Lock::Lock() :
		_mutex(SDL_CreateMutex()) {
}
#endif

Lock::~Lock() {
#ifdef TRACY_ENABLE
	delete _ctx;
#endif
	SDL_DestroyMutex(_mutex);
}

core_mutex* Lock::handle() {
	return _mutex;
}

void Lock::lock() const {
#ifdef TRACY_ENABLE
	const auto runAfter = _ctx->BeforeLock();
	SDL_LockMutex(_mutex);
	if (runAfter) {
		_ctx->AfterLock();
	}
#else
	SDL_LockMutex(_mutex);
#endif
}

void Lock::unlock() const {
#ifdef TRACY_ENABLE
	_ctx->BeforeUnlock();
#endif
	SDL_UnlockMutex(_mutex);
}

bool Lock::try_lock() const {
	const bool acquired = SDL_TryLockMutex(_mutex) == 0;
#ifdef TRACY_ENABLE
	_ctx->AfterTryLock(acquired);
#endif
	return acquired;
}

}
