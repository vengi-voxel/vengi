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

void TimeProvider::updateTickTime() {
	setTickTime(SDL_GetPerformanceCounter());
}

void TimeProvider::setTickTime(uint64_t tickTime) {
	_highResTime = tickTime;
	_tickSeconds = _highResTime / (double)SDL_GetPerformanceFrequency();
	_tickMillis = _highResTime / (double)(SDL_GetPerformanceFrequency() / (uint64_t)1000);
}

uint64_t TimeProvider::highResTimeResolution() {
	return SDL_GetPerformanceFrequency();
}

uint64_t TimeProvider::tickNow() const {
	return _highResTime / (SDL_GetPerformanceFrequency() / (uint64_t)1000);
}

uint64_t TimeProvider::systemMillis() {
	return SDL_GetPerformanceCounter() / (SDL_GetPerformanceFrequency() / (uint64_t)1000);
}

uint64_t TimeProvider::highResTime() {
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
