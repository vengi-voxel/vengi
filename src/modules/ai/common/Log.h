/**
 * @file
 */
#pragma once

#ifndef SIMPLEAI_SKIP_LOG

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace ai {
namespace Log {

#ifdef _MSC_VER
#define __attribute__(x)
#endif

static constexpr int bufSize = 1024;

static inline void trace(const char* msg, ...) __attribute__((format(printf, 1, 2)));
static inline void trace(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	std::vsnprintf(buf, sizeof(buf), msg, args);
	printf("TRACE: %s\n", buf);
	va_end(args);
}

static inline void debug(const char* msg, ...) __attribute__((format(printf, 1, 2)));
static inline void debug(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	std::vsnprintf(buf, sizeof(buf), msg, args);
	printf("DEBUG: %s\n", buf);
	va_end(args);
}

static inline void info(const char* msg, ...) __attribute__((format(printf, 1, 2)));
static inline void info(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	std::vsnprintf(buf, sizeof(buf), msg, args);
	printf("INFO: %s\n", buf);
	va_end(args);
}

static inline void warn(const char* msg, ...) __attribute__((format(printf, 1, 2)));
static inline void warn(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	std::vsnprintf(buf, sizeof(buf), msg, args);
	printf("WARN: %s\n", buf);
	va_end(args);
}

static inline void error(const char* msg, ...) __attribute__((format(printf, 1, 2)));
static inline void error(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	std::vsnprintf(buf, sizeof(buf), msg, args);
	printf("ERROR: %s\n", buf);
	va_end(args);
}

}
}
#endif
