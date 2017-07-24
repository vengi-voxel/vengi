/**
 * @file
 */

#pragma once

#include <cstdio>
#include <memory>

#ifndef __GNUC__
#define __attribute__(x)
#endif

#ifdef _MSC_VER
#include <sal.h>
#if _MSC_VER >= 1400
#define CORE_FORMAT_STRING _Printf_format_string_
#else
#define CORE_FORMAT_STRING __format_string
#endif
#else
#define CORE_FORMAT_STRING
#endif

class Log {
public:
	static void init();
	static void shutdown();
	static void trace(CORE_FORMAT_STRING const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void debug(CORE_FORMAT_STRING const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void info(CORE_FORMAT_STRING const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void warn(CORE_FORMAT_STRING const char* msg, ...) __attribute__((format(printf, 1, 2)));
	static void error(CORE_FORMAT_STRING const char* msg, ...) __attribute__((format(printf, 1, 2)));
};
