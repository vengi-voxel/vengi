/**
 * @file
 */

#include "ConditionVariable.h"
#include "core/concurrent/Lock.h"
#include <SDL_mutex.h>

namespace core {

ConditionVariable::ConditionVariable() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	_conditionVariable = SDL_CreateCondition();
#else
	_conditionVariable = SDL_CreateCond();
#endif
}

ConditionVariable::~ConditionVariable() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_DestroyCondition(_conditionVariable);
#else
	SDL_DestroyCond(_conditionVariable);
#endif
}

bool ConditionVariable::notify_one() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_SignalCondition(_conditionVariable);
	return true;
#else
	return SDL_CondSignal(_conditionVariable) == 0;
#endif
}

bool ConditionVariable::notify_all() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_BroadcastCondition(_conditionVariable);
	return true;
#else
	return SDL_CondBroadcast(_conditionVariable) == 0;
#endif
}

bool ConditionVariable::wait(Lock& lock) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_WaitCondition(_conditionVariable, lock.handle());
	return true;
#else
	return SDL_CondWait(_conditionVariable, lock.handle()) == 0;
#endif
}

ConditionVariableState ConditionVariable::waitTimeout(Lock& lock, uint32_t millis) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	const bool retVal = SDL_WaitConditionTimeout(_conditionVariable, lock.handle(), millis);
	if (retVal) {
		return ConditionVariableState::Signaled;
	}
#else
	const int retVal = SDL_CondWaitTimeout(_conditionVariable, lock.handle(), millis);
	if (retVal == 0) {
		return ConditionVariableState::Signaled;
	}
	if (retVal == -1) {
		return ConditionVariableState::Error;
	}
#endif
	return ConditionVariableState::Timeout;
}

}
