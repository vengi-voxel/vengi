/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Hash.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <SDL_log.h>

class Log {
public:
	enum class Level {
		None = 0,
		Trace = SDL_LOG_PRIORITY_VERBOSE,
		Debug = SDL_LOG_PRIORITY_DEBUG,
		Info = SDL_LOG_PRIORITY_INFO,
		Warn = SDL_LOG_PRIORITY_WARN,
		Error = SDL_LOG_PRIORITY_ERROR
	};

	static Level toLogLevel(const char* level);
	static const char* toLogLevel(Level level);

	static void init();
	static void shutdown();
	static void trace(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	static void debug(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	static void info(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	static void warn(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
	static void error(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);

	static bool enable(uint32_t id, Level level);
	static bool disable(uint32_t id);

	/**
	 * @brief Get log id for given name
	 */
	template<int N>
	static constexpr uint32_t logid(const char (&name)[N]) {
		return (uint32_t)core::hash(name);
	}

	static uint32_t logid(const char* name, int size) {
		return (uint32_t)core::hash(name, size);
	}

	static void trace(uint32_t id, CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(2);
	static void debug(uint32_t id, CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(2);
	static void info(uint32_t id, CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(2);
	static void warn(uint32_t id, CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(2);
	static void error(uint32_t id, CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(2);
};

extern "C" void c_logtrace(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_logdebug(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_loginfo(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_logwarn(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_logerror(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
extern "C" void c_logwrite(const char* msg, size_t length);
