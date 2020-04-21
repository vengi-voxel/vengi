/**
 * @file
 */

#pragma once

#include <stdint.h>

struct SDL_cond;

namespace core {

class Lock;

class ConditionVariable {
private:
	SDL_cond* _conditionVariable;
public:
	ConditionVariable();
	~ConditionVariable();

	bool signalOne();
	bool signalAll();
	bool wait(Lock& lock);
	bool waitTimeout(Lock& lock, uint32_t millis);
};

}
