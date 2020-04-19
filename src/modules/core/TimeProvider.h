/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <memory>

namespace core {

/**
 * @brief The time provider will get an updated tick time with every tick. It does not perform any timer system call on getting the
 * tick time - only if you really need the current time.
 */
class TimeProvider {
private:
	uint64_t _highResTime = 0u;
	double _tickMillis = 0.0;
	double _tickSeconds = 0.0;
public:
	uint64_t tickNow() const;

	/**
	 * @brief The tick time gives you the time in milliseconds when the tick was started.
	 * @note Updated once per tick
	 */
	inline double tickMillis() const {
		return _tickMillis;
	}

	/**
	 * @brief The tick time gives you the time in seconds when the tick was started.
	 * @note Updated once per tick
	 */
	inline double tickSeconds() const {
		return _tickSeconds;
	}

	static core::String toString(unsigned long millis, const char *format = "%d-%m-%Y %H-%M-%S");

	static uint64_t systemMillis();
	static uint64_t highResTime();
	static uint64_t highResTimeResolution();

	/**
	 * @brief The raw high res tick time in whatever resolution the platform provides it. This is a cached value
	 * that is stored at the beginning of the frame
	 * @sa highResTime()
	 * @sa highResTimeResolution()
	 */
	uint64_t highResTickTime() const {
		return _highResTime;
	}

	void updateTickTime();
	void setTickTime(uint64_t tickMillis);
};

typedef std::shared_ptr<TimeProvider> TimeProviderPtr;

}
