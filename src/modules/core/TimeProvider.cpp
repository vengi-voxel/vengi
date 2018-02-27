/**
 * @file
 */

#include "TimeProvider.h"
#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <time.h>
#include <sys/time.h>

namespace core {

TimeProvider::TimeProvider() :
		_tickMillis(uint64_t(0)) {
}

uint64_t TimeProvider::systemMillis() const {
	auto unix_timestamp = std::chrono::seconds(std::time(NULL));
	const uint64_t millis = unix_timestamp.count() * 1000UL;
	timeval curTime;
	gettimeofday(&curTime, nullptr);
	return millis + curTime.tv_usec / 1000L;
}

double TimeProvider::systemNanos() {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1e9;
}

std::string TimeProvider::toString(unsigned long millis, const char *format) {
	time_t t(millis / 1000UL);
	tm tm = *gmtime(&t);
	std::stringstream ss;
	ss << std::put_time(&tm, format);
	return ss.str();
}

}
