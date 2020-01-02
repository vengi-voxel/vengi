/**
 * @file
 */

#include "Log.h"
#include "Var.h"
#include "String.h"
#include "engine-config.h"
#include "App.h"
#include "Common.h"
#include "Array.h"
#include "Assert.h"
#include <string.h>
#include <stdio.h>
#include <unordered_map>

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
static std::unordered_map<uint32_t, int> _logActive;

#ifdef HAVE_SYSLOG_H
static SDL_LogOutputFunction _sdlCallback = nullptr;
static void *_sdlCallbackUserData = nullptr;


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
	if (_sdlCallback != sysLogOutputFunction) {
		_sdlCallback(_sdlCallbackUserData, category, priority, message);
	}
}
#endif

Log::Level Log::toLogLevel(const std::string& string) {
	if (core::string::iequals(string, "trace")) {
		return Level::Trace;
	}
	if (core::string::iequals(string, "debug")) {
		return Level::Debug;
	}
	if (core::string::iequals(string, "info")) {
		return Level::Info;
	}
	if (core::string::iequals(string, "warn")) {
		return Level::Warn;
	}
	if (core::string::iequals(string, "error")) {
		return Level::Error;
	}
	return Level::None;
}

const char* Log::toLogLevel(Log::Level level) {
	if (level == Level::Trace) {
		return "trace";
	}
	if (level == Level::Debug) {
		return "debug";
	}
	if (level == Level::Info) {
		return "info";
	}
	if (level == Level::Warn) {
		return "warn";
	}
	if (level == Level::Error) {
		return "error";
	}
	return "none";
}

void Log::init() {
	_logLevel = (SDL_LogPriority)core::Var::getSafe(cfg::CoreLogLevel)->intVal();
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, _logLevel);

	const bool syslog = core::Var::getSafe(cfg::CoreSysLog)->boolVal();
	if (syslog) {
#ifdef HAVE_SYSLOG_H
		if (!_syslog) {
			if (_sdlCallback == nullptr) {
				SDL_LogGetOutputFunction(&_sdlCallback, &_sdlCallbackUserData);
			}
			core_assert(_sdlCallback != sysLogOutputFunction);
			openlog(nullptr, LOG_PID, LOG_USER);
			SDL_LogSetOutputFunction(sysLogOutputFunction, nullptr);
			_syslog = true;
		}
#else
		Log::warn("Syslog support is not compiled into the binary");
		_syslog = false;
#endif
	} else {
#ifdef HAVE_SYSLOG_H
		if (_syslog) {
			SDL_LogSetOutputFunction(_sdlCallback, _sdlCallbackUserData);
			_sdlCallback = nullptr;
			_sdlCallbackUserData = nullptr;
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
	if (_syslog) {
		SDL_LogSetOutputFunction(_sdlCallback, _sdlCallbackUserData);
		closelog();
		_sdlCallback = nullptr;
		_sdlCallbackUserData = nullptr;
	}
#endif
	_logActive.clear();
	_logLevel = SDL_LOG_PRIORITY_INFO;
	_syslog = false;
}

static void traceVA(uint32_t id, const char *msg, va_list args) {
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (_syslog) {
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
	va_end(args);
}

static void debugVA(uint32_t id, const char *msg, va_list args) {
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (_syslog) {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
	va_end(args);
}

static void infoVA(uint32_t id, const char *msg, va_list args) {
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (_syslog) {
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
	va_end(args);
}

static void warnVA(uint32_t id, const char *msg, va_list args) {
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (_syslog) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
	va_end(args);
}

static void errorVA(uint32_t id, const char *msg, va_list args) {
	char buf[bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (_syslog) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
	va_end(args);
}

void Log::trace(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_VERBOSE) {
		return;
	}
	va_list args;
	va_start(args, msg);
	traceVA(0u, msg, args);
}

void Log::debug(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_DEBUG) {
		return;
	}
	va_list args;
	va_start(args, msg);
	debugVA(0u, msg, args);
}

void Log::info(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_INFO) {
		return;
	}
	va_list args;
	va_start(args, msg);
	infoVA(0u, msg, args);
}

void Log::warn(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_WARN) {
		return;
	}
	va_list args;
	va_start(args, msg);
	warnVA(0u, msg, args);
}

void Log::error(const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_ERROR) {
		return;
	}
	va_list args;
	va_start(args, msg);
	errorVA(0u, msg, args);
}

void Log::trace(uint32_t id, const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_VERBOSE) {
		auto i = _logActive.find(id);
		if (i == _logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_VERBOSE) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	traceVA(id, msg, args);
}

void Log::debug(uint32_t id, const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_DEBUG) {
		auto i = _logActive.find(id);
		if (i == _logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_DEBUG) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	debugVA(id, msg, args);
}

void Log::info(uint32_t id, const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_INFO) {
		auto i = _logActive.find(id);
		if (i == _logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_INFO) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	infoVA(id, msg, args);
}

void Log::warn(uint32_t id, const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_WARN) {
		auto i = _logActive.find(id);
		if (i == _logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_WARN) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	warnVA(id, msg, args);
}

void Log::error(uint32_t id, const char* msg, ...) {
	if (_logLevel > SDL_LOG_PRIORITY_ERROR) {
		auto i = _logActive.find(id);
		if (i == _logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_ERROR) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	errorVA(id, msg, args);
}

bool Log::enable(uint32_t id, Log::Level level) {
	return _logActive.insert(std::make_pair(id, std::enum_value(level))).second;
}

bool Log::disable(uint32_t id) {
	return _logActive.erase(id) == 1;
}
