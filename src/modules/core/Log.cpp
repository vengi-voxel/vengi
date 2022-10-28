/**
 * @file
 */

#include "core/Log.h"
#include "core/Var.h"
#include "core/StringUtil.h"
#include "engine-config.h"
#include "core/Common.h"
#include "core/Enum.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
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

namespace priv {

static bool _syslog = false;
static FILE* _logfile = nullptr;
static constexpr int bufSize = 4096;
static SDL_LogPriority _logLevel = SDL_LOG_PRIORITY_INFO;
static std::unordered_map<uint32_t, int> _logActive;

#ifdef HAVE_SYSLOG_H
static SDL_LogOutputFunction _syslogLogCallback = nullptr;
static void *_syslogLogCallbackUserData = nullptr;


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
	if (_syslogLogCallback != sysLogOutputFunction) {
		_syslogLogCallback(_syslogLogCallbackUserData, category, priority, message);
	}
}
#endif
}

void Log::setLogLevel(Level level) {
	priv::_logLevel = (SDL_LogPriority)core::enumVal(level);
}

Log::Level Log::toLogLevel(const char* level) {
	const core::String string(level);
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

void Log::init(const char *logfile) {
	priv::_logLevel = (SDL_LogPriority)core::Var::getSafe(cfg::CoreLogLevel)->intVal();
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE);

	if (priv::_logfile == nullptr && logfile != nullptr) {
		priv::_logfile = fopen(logfile, "w");
	}

	const bool syslog = core::Var::getSafe(cfg::CoreSysLog)->boolVal();
	if (syslog) {
#ifdef HAVE_SYSLOG_H
		if (!priv::_syslog) {
			if (priv::_syslogLogCallback == nullptr) {
				SDL_LogGetOutputFunction(&priv::_syslogLogCallback, &priv::_syslogLogCallbackUserData);
			}
			core_assert(priv::_syslogLogCallback != priv::sysLogOutputFunction);
			openlog(nullptr, LOG_PID, LOG_USER);
			SDL_LogSetOutputFunction(priv::sysLogOutputFunction, nullptr);
			priv::_syslog = true;
		}
#else
		Log::warn("Syslog support is not compiled into the binary");
		priv::_syslog = false;
#endif
	} else {
#ifdef HAVE_SYSLOG_H
		if (priv::_syslog) {
			SDL_LogSetOutputFunction(priv::_syslogLogCallback, priv::_syslogLogCallbackUserData);
			priv::_syslogLogCallback = nullptr;
			priv::_syslogLogCallbackUserData = nullptr;
			closelog();
		}
#endif
		priv::_syslog = false;
	}
}

void Log::shutdown() {
	// this is one of the last methods that is executed - so don't rely on anything
	// still being available here - it won't
#ifdef HAVE_SYSLOG_H
	if (priv::_syslog) {
		SDL_LogSetOutputFunction(priv::_syslogLogCallback, priv::_syslogLogCallbackUserData);
		closelog();
		priv::_syslogLogCallback = nullptr;
		priv::_syslogLogCallbackUserData = nullptr;
	}
#endif
	if (priv::_logfile) {
		fflush(priv::_logfile);
		fclose(priv::_logfile);
		priv::_logfile = nullptr;
	}
	priv::_logActive.clear();
	priv::_logLevel = SDL_LOG_PRIORITY_INFO;
	priv::_syslog = false;
}

static void traceVA(uint32_t id, const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[TRACE] (%u) %s\n", id, buf);
	}
	if (priv::_syslog) {
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
}

static void debugVA(uint32_t id, const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[DEBUG] (%u) %s\n", id, buf);
	}
	if (priv::_syslog) {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
}

static void infoVA(uint32_t id, const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[INFO] (%u) %s\n", id, buf);
	}
	if (priv::_syslog) {
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
}

static void warnVA(uint32_t id, const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[WARN] (%u) %s\n", id, buf);
	}
	if (priv::_syslog) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
}

static void errorVA(uint32_t id, const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[ERROR] (%u) %s\n", id, buf);
	}
	if (priv::_syslog) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "(%u) %s\n", id, buf);
	} else {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "(%u) " ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", id, buf);
	}
}

void Log::trace(const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_VERBOSE) {
		return;
	}
	va_list args;
	va_start(args, msg);
	traceVA(0u, msg, args);
	va_end(args);
}

void Log::debug(const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_DEBUG) {
		return;
	}
	va_list args;
	va_start(args, msg);
	debugVA(0u, msg, args);
	va_end(args);
}

void Log::info(const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_INFO) {
		return;
	}
	va_list args;
	va_start(args, msg);
	infoVA(0u, msg, args);
	va_end(args);
}

void Log::warn(const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_WARN) {
		return;
	}
	va_list args;
	va_start(args, msg);
	warnVA(0u, msg, args);
	va_end(args);
}

void Log::error(const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_ERROR) {
		return;
	}
	va_list args;
	va_start(args, msg);
	errorVA(0u, msg, args);
	va_end(args);
}

void Log::trace(uint32_t id, const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_VERBOSE) {
		auto i = priv::_logActive.find(id);
		if (i == priv::_logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_VERBOSE) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	traceVA(id, msg, args);
	va_end(args);
}

void Log::debug(uint32_t id, const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_DEBUG) {
		auto i = priv::_logActive.find(id);
		if (i == priv::_logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_DEBUG) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	debugVA(id, msg, args);
	va_end(args);
}

void Log::info(uint32_t id, const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_INFO) {
		auto i = priv::_logActive.find(id);
		if (i == priv::_logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_INFO) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	infoVA(id, msg, args);
	va_end(args);
}

void Log::warn(uint32_t id, const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_WARN) {
		auto i = priv::_logActive.find(id);
		if (i == priv::_logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_WARN) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	warnVA(id, msg, args);
	va_end(args);
}

void Log::error(uint32_t id, const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_ERROR) {
		auto i = priv::_logActive.find(id);
		if (i == priv::_logActive.end()) {
			return;
		}
		if (i->second > SDL_LOG_PRIORITY_ERROR) {
			return;
		}
	}
	va_list args;
	va_start(args, msg);
	errorVA(id, msg, args);
	va_end(args);
}

bool Log::enable(uint32_t id, Log::Level level) {
	return priv::_logActive.insert(std::make_pair(id, core::enumVal(level))).second;
}

bool Log::disable(uint32_t id) {
	return priv::_logActive.erase(id) == 1;
}

void c_logtrace(const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_VERBOSE) {
		return;
	}
	va_list args;
	va_start(args, msg);
	traceVA(0u, msg, args);
	va_end(args);
}

void c_logdebug(const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_DEBUG) {
		return;
	}
	va_list args;
	va_start(args, msg);
	debugVA(0u, msg, args);
	va_end(args);
}

void c_loginfo(const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_INFO) {
		return;
	}
	va_list args;
	va_start(args, msg);
	infoVA(0u, msg, args);
	va_end(args);
}

void c_logwarn(const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_WARN) {
		return;
	}
	va_list args;
	va_start(args, msg);
	warnVA(0u, msg, args);
	va_end(args);
}

void c_logerror(CORE_FORMAT_STRING const char* msg, ...) {
	if (priv::_logLevel > SDL_LOG_PRIORITY_ERROR) {
		return;
	}
	va_list args;
	va_start(args, msg);
	errorVA(0u, msg, args);
	va_end(args);
}

extern "C" void c_logwrite(const char* msg, size_t length) {
	char buf[priv::bufSize];
	SDL_memcpy(buf, msg, core_max(priv::bufSize - 1, length));
	buf[sizeof(buf) - 1] = '\0';
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", buf);
}
