/**
 * @file
 */

#include "Semaphore.h"
#include <SDL_mutex.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDL_SemTryWait SDL_TryWaitSemaphore
#define SDL_SemValue SDL_GetSemaphoreValue
#endif

namespace core {

Semaphore::Semaphore(uint32_t initialValue) :
		_semaphore(SDL_CreateSemaphore(initialValue)) {
}

Semaphore::~Semaphore() {
	SDL_DestroySemaphore(_semaphore);
}

bool Semaphore::waitAndDecrease() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_WaitSemaphore(_semaphore);
	return true;
#else
	return SDL_SemWait(_semaphore) != -1;
#endif
}

bool Semaphore::increase() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_SignalSemaphore(_semaphore);
	return true;
#else
	return SDL_SemPost(_semaphore) != -1;
#endif
}

SemaphoreWaitState Semaphore::tryWait() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	const bool val = SDL_TryWaitSemaphore(_semaphore);
	if (!val) {
		return SemaphoreWaitState::WouldBlock;
	}
#else
	const int val = SDL_SemTryWait(_semaphore);
	if (val == -1) {
		return SemaphoreWaitState::Error;
	}
	if (val == SDL_MUTEX_TIMEDOUT) {
		return SemaphoreWaitState::WouldBlock;
	}
#endif
	return SemaphoreWaitState::Success;
}

bool Semaphore::waitTimeout(uint32_t timeout) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	return SDL_WaitSemaphoreTimeout(_semaphore, timeout);
#else
	return SDL_SemWaitTimeout(_semaphore, timeout) != -1;
#endif
}

uint32_t Semaphore::value() {
	return SDL_SemValue(_semaphore);
}

}
