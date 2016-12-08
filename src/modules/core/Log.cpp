/**
 * @file
 */

#include "Log.h"
#include "Var.h"
#include <cstring>
#include <SDL.h>

#ifdef __LINUX__
#define ANSI_COLOR_RESET "\033[0m"
#define ANSI_COLOR_RED "\033[31m"
#define ANSI_COLOR_GREEN "\033[32m"
#define ANSI_COLOR_YELLOW "\033[33m"
#define ANSI_COLOR_BLUE "\033[34m"
#define ANSI_COLOR_CYAN "\033[36m"
#else
#define ANSI_COLOR_RESET ""
#define ANSI_COLOR_RED ""
#define ANSI_COLOR_GREEN ""
#define ANSI_COLOR_YELLOW ""
#define ANSI_COLOR_BLUE ""
#define ANSI_COLOR_CYAN ""
#endif


static constexpr int bufSize = 1024;
static SDL_LogPriority _logLevel = SDL_LOG_PRIORITY_INFO;

void Log::init() {
	_logLevel = (SDL_LogPriority)core::Var::get(cfg::CoreLogLevel, SDL_LOG_PRIORITY_INFO)->intVal();
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, _logLevel);
}

void Log::trace(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_VERBOSE) {
		return;
	}
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET "\n", buf);
	va_end(args);
}

void Log::debug(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_DEBUG) {
		return;
	}
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET "\n", buf);
	va_end(args);
}

void Log::info(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_INFO) {
		return;
	}
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET "\n", buf);
	va_end(args);
}

void Log::warn(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_WARN) {
		return;
	}
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", buf);
	va_end(args);
}

void Log::error(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_ERROR) {
		return;
	}
	va_list args;
	va_start(args, msg);
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", buf);
	va_end(args);
}
