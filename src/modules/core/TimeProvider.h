/**
 * @file
 */

#pragma once

#include <string>
#include <memory>

namespace core {

/**
 * The time provider will get an updated tick time with every tick. It does not perform any timer system call on getting the
 * tick time - only if you really need the current time.
 */
class TimeProvider {
private:
	uint64_t _tickMillis;
public:
	TimeProvider();

	/**
	 * @brief The tick time gives you the time in milliseconds when the tick was started.
	 * @note Updated once per tick
	 */
	inline uint64_t tickMillis() const {
		return _tickMillis;
	}

	inline uint64_t tickSeconds() const {
		return _tickMillis / uint64_t(1000);
	}

	static std::string toString(unsigned long millis, const char *format = "%d-%m-%Y %H-%M-%S");

	uint64_t systemMillis() const;

	static double systemNanos();

	void update(uint64_t tickTime) {
		_tickMillis = tickTime;
	}
};

typedef std::shared_ptr<TimeProvider> TimeProviderPtr;

}
