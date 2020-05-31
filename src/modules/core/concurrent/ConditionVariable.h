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

	bool notify_one();
	bool notify_all();
	bool wait(Lock& lock);

	template<class PREDICATE>
	void wait(Lock& lock, PREDICATE&& predicate) {
		while (!predicate()) {
			wait(lock);
		}
	}

	bool waitTimeout(Lock& lock, uint32_t millis);
};

}
