/**
 * @file
 */

#pragma once

#include <stdint.h>

struct SDL_cond;

namespace core {

class Lock;

enum class ConditionVariableState {
	Signaled,
	Timeout,
	Error
};

class ConditionVariable {
private:
	SDL_cond* _conditionVariable;
public:
	ConditionVariable();
	~ConditionVariable();

	bool notify_one();
	bool notify_all();
	bool wait(Lock& lock);

	/**
	 * @brief Predicate must return false if the waiting should continue
	 */
	template<class PREDICATE>
	void wait(Lock& lock, PREDICATE&& predicate, uint32_t millis = 0u) {
		while (!predicate()) {
			if (millis == 0u) {
				wait(lock);
			} else {
				waitTimeout(lock, millis);
			}
		}
	}

	ConditionVariableState waitTimeout(Lock& lock, uint32_t millis);
};

}
