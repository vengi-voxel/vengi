/**
 * @file
 */

#pragma once

#include <memory>
#include <chrono>

namespace core {

/**
 * The time provider will get an updated tick time with every tick. It does not perform any timer system call on getting the
 * tick time - only if you really need the current time.
 */
class TimeProvider {
private:
	unsigned long _tickTime;
public:
	TimeProvider();

	/**
	 * @brief The tick time gives you the time in milliseconds when the tick was started.
	 * @note Updated once per tick
	 */
	inline unsigned long tickTime() const {
		return _tickTime;
	}

	unsigned long currentTime() const {
		const unsigned long now = (unsigned long) std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
		return now;
	}

	static inline double currentNanos() {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1e9;
	}

	void update(unsigned long now) {
		_tickTime = now;
	}
};

typedef std::shared_ptr<TimeProvider> TimeProviderPtr;

}
