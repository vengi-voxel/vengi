/**
 * @file
 */

#include "TimeProvider.h"
#include "core/Assert.h"
#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <time.h>
#include <inttypes.h>
#include <SDL.h>

#ifdef __WINDOWS__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64

#ifndef _WINSOCK2API_
// MSVC defines this in winsock2.h!?
typedef struct timeval {
	long tv_sec;
	long tv_usec;
} timeval;
#endif

int gettimeofday(struct timeval * tp, struct timezone * tzp) {
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970
	static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

	SYSTEMTIME system_time;
	FILETIME file_time;
	uint64_t time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long) ((time - EPOCH) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
	return 0;
}
#else
#include <sys/time.h>
#endif

namespace core {

TimeProvider::TimeProvider() :
		_tickMillis(uint64_t(0)) {
}

uint64_t TimeProvider::systemMillis() const {
#if 1
	struct timeval tp;
	gettimeofday(&tp, nullptr);
	uint64_t timestamp = tp.tv_sec * 1000 + tp.tv_usec / 1000;
#else
	const uint64_t timestamp = SDL_GetPerformanceCounter() / 1000000UL;
#endif
#ifdef DEBUG
	static uint64_t last = 0;
#endif
#ifdef DEBUG
	core_assert_msg(timestamp >= last,
			"timestamp:%" PRIu64 ", last: %" PRIu64,
			timestamp, last);
	last = timestamp;
#endif
	return timestamp;
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
