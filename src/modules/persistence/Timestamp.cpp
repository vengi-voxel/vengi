/**
 * @file
 */

#include "Timestamp.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <time.h>

namespace persistence {

Timestamp::Timestamp(uint64_t seconds) :
		_seconds(seconds), _now(false) {
}

Timestamp::Timestamp() :
	Timestamp(0L) {
}

core::String Timestamp::toString(const char *format) const {
	std::time_t t(_seconds);
	std::tm tm = *std::gmtime(&t);
	std::stringstream ss;
	ss << std::put_time(&tm, format);
	return ss.str();
}

Timestamp Timestamp::now() {
	static Timestamp nowInstance;
	nowInstance._now = true;
	return nowInstance;
}

}
