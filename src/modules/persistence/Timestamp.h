/**
 * @file
 */

#pragma once

#include <cstdint>

namespace persistence {

class Timestamp {
private:
	uint64_t _time;
	bool _now;
public:
	Timestamp(uint64_t time);
	Timestamp();

	static Timestamp now();

	bool isNow() const;

	uint64_t time() const;
};

}
