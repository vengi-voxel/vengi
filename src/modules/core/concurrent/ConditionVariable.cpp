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

bool ConditionVariable::notify_one() {
	return SDL_CondSignal(_conditionVariable) != -1;
}

bool ConditionVariable::notify_all() {
	return SDL_CondBroadcast(_conditionVariable) != -1;
}

bool ConditionVariable::wait(Lock& lock) {
	return SDL_CondWait(_conditionVariable, lock.handle()) != -1;
}

ConditionVariableState ConditionVariable::waitTimeout(Lock& lock, uint32_t millis) {
	const int retVal = SDL_CondWaitTimeout(_conditionVariable, lock.handle(), millis);
	if (retVal == 0) {
		return ConditionVariableState::Signaled;
	}
	if (retVal == -1) {
		return ConditionVariableState::Error;
	}
	return ConditionVariableState::Timeout;
}

}
