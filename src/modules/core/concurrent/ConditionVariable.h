/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
struct SDL_Condition;
using core_condition = SDL_Condition;
#else
struct SDL_cond;
using core_condition = SDL_cond;
#endif

namespace core {

class Lock;

enum class ConditionVariableState {
	Signaled,
	Timeout,
	Error
};

class ConditionVariable {
private:
	core_condition* _conditionVariable;;
public:
	ConditionVariable();
	~ConditionVariable();

	bool notify_one();
	bool notify_all();
	/**
	 * @brief Wait for the condition to get signaled
	 * @note Keep in mind that you have to wrap this in a predicate check to
	 * work around spurious wakeups.
	 */
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
