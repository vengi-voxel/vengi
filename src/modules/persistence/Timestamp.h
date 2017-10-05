/**
 * @file
 */

#pragma once

#include <cstdint>

namespace persistence {

class Timestamp {
private:
	uint64_t _millis;
	bool _now;
public:
	Timestamp(uint64_t millis);
	Timestamp();

	static Timestamp now();

	bool isNow() const;

	uint64_t time() const;

	uint64_t millis() const;
};

inline bool Timestamp::isNow() const {
	return _now;
}

inline uint64_t Timestamp::time() const {
	return _millis;
}

inline uint64_t Timestamp::millis() const {
	return _millis;
}

}
