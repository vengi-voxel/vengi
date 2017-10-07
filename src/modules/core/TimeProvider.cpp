/**
 * @file
 */

#include "TimeProvider.h"
#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>

namespace core {

TimeProvider::TimeProvider() :
		_tickTime(0ul) {
}

unsigned long TimeProvider::currentTime() const {
	const unsigned long now = (unsigned long)(std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1));
	return now;
}

double TimeProvider::currentNanos() {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1e9;
}

std::string TimeProvider::toString(unsigned long millis, const char *format) {
	std::time_t t(millis / 1000UL);
	std::tm tm = *std::gmtime(&t);
	std::stringstream ss;
	ss << std::put_time(&tm, format);
	return ss.str();
}

}
