/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include <string.h>
#include <stdio.h>

class Log {
public:
	enum class Level { None, Trace, Debug, Info, Warn, Error };

	static Level toLogLevel(const char* level);
	static const char* toLogLevel(Level level);

	static void init(const char *logfile = nullptr);
	static void shutdown();
	static void trace(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	static void debug(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	static void info(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	static void warn(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	static void error(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	// the only version that doesn't add a newline
	static void printf(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	static void setLevel(Level level);
	static void setConsoleColors(bool enabled);
};

extern "C" void c_logtrace(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_logdebug(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_loginfo(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_logwarn(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_logerror(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_logwrite(const char* msg, size_t length);
