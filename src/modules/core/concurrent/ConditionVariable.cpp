/**
 * @file
 */

#include "ConditionVariable.h"
#include "core/concurrent/Lock.h"
#include <SDL_mutex.h>

namespace core {

ConditionVariable::ConditionVariable() {
	_conditionVariable = SDL_CreateCond();
}

ConditionVariable::~ConditionVariable() {
	SDL_DestroyCond(_conditionVariable);
}

bool ConditionVariable::signalOne() {
	return SDL_CondSignal(_conditionVariable) != -1;
}

bool ConditionVariable::signalAll() {
	return SDL_CondBroadcast(_conditionVariable) != -1;
}

bool ConditionVariable::wait(Lock& lock) {
	return SDL_CondWait(_conditionVariable, lock.handle()) != -1;
}

bool ConditionVariable::waitTimeout(Lock& lock, uint32_t millis) {
	return SDL_CondWaitTimeout(_conditionVariable, lock.handle(), millis) == -1;
}

}
