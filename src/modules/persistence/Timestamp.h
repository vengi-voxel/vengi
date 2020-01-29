/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <stdint.h>

namespace persistence {

/**
 * @brief Represents timestamp values in seconds.
 *
 * @ingroup Persistence
 */
class Timestamp {
private:
	uint64_t _seconds;
	bool _now;
public:
	/**
	 * Unix epoch (seconds since 1970-01-01 00:00:00+00)
	 */
	Timestamp(uint64_t seconds);
	Timestamp();

	static Timestamp now();
	bool isNow() const;

	core::String toString(const char *format = "%d-%m-%Y %H-%M-%S") const;

	uint64_t seconds() const;
	uint64_t millis() const;
};

inline bool Timestamp::isNow() const {
	return _now;
}

inline uint64_t Timestamp::seconds() const {
	return _seconds;
}

inline uint64_t Timestamp::millis() const {
	return _seconds * (uint64_t)1000;
}

}
