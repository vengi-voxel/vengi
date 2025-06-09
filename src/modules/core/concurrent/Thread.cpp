/**
 * @file
 */

#include "Thread.h"
#include <SDL_thread.h>
#include <SDL_version.h>

namespace core {

ThreadId getCurrentThreadId() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	return SDL_GetCurrentThreadID();
#else
	return SDL_ThreadID();
#endif
}

}
