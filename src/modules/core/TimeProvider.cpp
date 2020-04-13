/**
 * @file
 */

#include "TimeProvider.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <inttypes.h>
#include <SDL.h>

namespace core {

TimeProvider::TimeProvider() :
		_tickMillis(uint64_t(0)) {
}

uint64_t TimeProvider::systemMillis() const {
	return systemNanos() / 1000000;
}

uint64_t TimeProvider::systemNanos() {
	return SDL_GetPerformanceCounter();
}

core::String TimeProvider::toString(unsigned long millis, const char *format) {
	time_t t(millis / 1000UL);
	tm tm = *gmtime(&t);
	std::stringstream ss;
	ss << std::put_time(&tm, format);
	const std::string& tmp = ss.str();
	return core::String(tmp.c_str());
}

}
