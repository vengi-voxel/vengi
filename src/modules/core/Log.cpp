/**
 * @file
 */

#include "Log.h"
#include "Var.h"
#include "engine-config.h"
#include "App.h"
#include <cstring>
#include <cstdio>
#include <SDL.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

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

static bool _syslog = false;
static constexpr int bufSize = 4096;
static SDL_LogPriority _logLevel = SDL_LOG_PRIORITY_INFO;

#ifdef HAVE_SYSLOG_H
static void sysLogOutputFunction(void *userdata, int category, SDL_LogPriority priority, const char *message) {
	int syslogLevel = LOG_DEBUG;
	if (priority == SDL_LOG_PRIORITY_CRITICAL) {
		syslogLevel = LOG_CRIT;
	} else if (priority == SDL_LOG_PRIORITY_ERROR) {
		syslogLevel = LOG_ERR;
	} else if (priority == SDL_LOG_PRIORITY_WARN) {
		syslogLevel = LOG_WARNING;
	} else if (priority == SDL_LOG_PRIORITY_INFO) {
		syslogLevel = LOG_INFO;
	}
	syslog(syslogLevel, "%s", message);
	if (priority == SDL_LOG_PRIORITY_CRITICAL || priority == SDL_LOG_PRIORITY_ERROR) {
		fprintf(stderr, "%s\n", message);
		fflush(stderr);
	}
}
#endif

void Log::init() {
	_logLevel = (SDL_LogPriority)core::Var::getSafe(cfg::CoreLogLevel)->intVal();
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, _logLevel);

	const bool syslog = core::Var::getSafe(cfg::CoreSysLog)->boolVal();
	if (syslog) {
#ifdef HAVE_SYSLOG_H
		openlog(nullptr, LOG_PID, LOG_USER);
		SDL_LogSetOutputFunction(sysLogOutputFunction, nullptr);
		_syslog = true;
#else
		Log::warn("Syslog support is not compiled into the binary");
		_syslog = false;
#endif
	} else {
#ifdef HAVE_SYSLOG_H
		if (_syslog) {
			closelog();
		}
#endif
		_syslog = false;
	}
}

void Log::shutdown() {
	// this is one of the last methods that is executed - so don't rely on anything
	// still being available here - it won't
#ifdef HAVE_SYSLOG_H
	closelog();
#endif
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
	if (_syslog) {
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
	} else {
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET "\n", buf);
	}
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
	if (_syslog) {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
	} else {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET "\n", buf);
	}
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
	if (_syslog) {
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
	} else {
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET "\n", buf);
	}
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
	if (_syslog) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
	} else {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", buf);
	}
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
	if (_syslog) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
	} else {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", buf);
	}
	va_end(args);
}
