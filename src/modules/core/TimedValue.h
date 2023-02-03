/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace core {

/**
 * @brief Make a particular value valid until the time ran out
 */
template <class T> class TimedValue {
private:
	T _value;
	uint64_t _start;
	uint64_t _end;

public:
	TimedValue() : _start(0), _end(0) {
	}

	TimedValue(const T &val, uint64_t start, uint64_t duration) : _value(val), _start(start), _end(start + duration) {
	}

	bool isValid(uint64_t now) const {
		return _end > now;
	}

	uint64_t remaining(uint64_t now) const {
		if (!isValid(now)) {
			return 0;
		}
		return _end - now;
	}

	const T &value() const {
		return _value;
	}

	T &value() {
		return _value;
	}
};

} // namespace core
