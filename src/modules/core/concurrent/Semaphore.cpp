/**
 * @file
 */

#include "Semaphore.h"
#include <SDL_mutex.h>

namespace core {

Semaphore::Semaphore(uint32_t initialValue) :
		_semaphore(SDL_CreateSemaphore(initialValue)) {
}

Semaphore::~Semaphore() {
	SDL_DestroySemaphore(_semaphore);
}

bool Semaphore::waitAndDecrease() {
	return SDL_SemWait(_semaphore) != -1;
}

bool Semaphore::increase() {
	return SDL_SemPost(_semaphore) != -1;
}

SemaphoreWaitState Semaphore::tryWait() {
	const int val = SDL_SemTryWait(_semaphore);
	if (val == -1) {
		return SemaphoreWaitState::Error;
	}
	if (val == SDL_MUTEX_TIMEDOUT) {
		return SemaphoreWaitState::WouldBlock;
	}
	return SemaphoreWaitState::Success;
}

bool Semaphore::waitTimeout(uint32_t timeout) {
	return SDL_SemWaitTimeout(_semaphore, timeout) != -1;
}

uint32_t Semaphore::value() {
	return SDL_SemValue(_semaphore);
}

}
