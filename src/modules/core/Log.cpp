/**
 * @file
 */

#include "core/Log.h"
#include "core/Var.h"
#include "core/StringUtil.h"
#include "engine-config.h"
#include "core/Common.h"
#include "core/Assert.h"
#include <string.h>
#include <stdio.h>
#include <SDL_log.h>
#include <SDL_version.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDL_LogSetPriority SDL_SetLogPriority
#define SDL_LogGetOutputFunction SDL_GetLogOutputFunction
#define SDL_LogSetOutputFunction SDL_SetLogOutputFunction
#endif

namespace priv {
#if defined(_WIN32)
static WORD defaultAttributes = 0;
#endif

static FILE* _logfile = nullptr;
static constexpr int bufSize = 4096;
static Log::Level _logLevel = Log::Level::Info;

static SDL_LogOutputFunction _logCallback = nullptr;
static void *_logCallbackUserData = nullptr;
static bool _logConsoleColors = true;

static Log::ILogListener *_logListener = nullptr;

static void logOutputFunction(void *userdata, int category, SDL_LogPriority priority, const char *message) {
	if (_logCallback == nullptr) {
		return;
	}
#if defined(__linux__) || defined(__APPLE__)
	char buf[priv::bufSize + 16]; // additional ansi color codes
	#define ANSI_COLOR_RESET "\033[00m"
	#define ANSI_COLOR_RED "\033[31m"
	#define ANSI_COLOR_GREEN "\033[32m"
	#define ANSI_COLOR_YELLOW "\033[33m"
	#define ANSI_COLOR_BLUE "\033[34m"
	#define ANSI_COLOR_CYAN "\033[36m"
	if (_logConsoleColors && priority == SDL_LOG_PRIORITY_VERBOSE) {
		core::String::formatBuf(buf, sizeof(buf), ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET, message);
	} else if (_logConsoleColors && priority == SDL_LOG_PRIORITY_DEBUG) {
		core::String::formatBuf(buf, sizeof(buf), ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET, message);
	} else if (_logConsoleColors && priority == SDL_LOG_PRIORITY_INFO) {
		core::String::formatBuf(buf, sizeof(buf), ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET, message);
	} else if (_logConsoleColors && priority == SDL_LOG_PRIORITY_WARN) {
		core::String::formatBuf(buf, sizeof(buf), ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET, message);
	} else if (_logConsoleColors && (priority == SDL_LOG_PRIORITY_ERROR || priority == SDL_LOG_PRIORITY_CRITICAL)) {
		core::String::formatBuf(buf, sizeof(buf), ANSI_COLOR_RED "%s" ANSI_COLOR_RESET, message);
	} else {
		core::String::formatBuf(buf, sizeof(buf),  "%s", message);
	}
	_logCallback(_logCallbackUserData, category, priority, buf);
#elif defined(_WIN32)
	int color = -1;
	if (_logConsoleColors && priority == SDL_LOG_PRIORITY_VERBOSE) {
		color = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
	} else if (_logConsoleColors && priority == SDL_LOG_PRIORITY_DEBUG) {
		color = FOREGROUND_BLUE;
	} else if (_logConsoleColors && priority == SDL_LOG_PRIORITY_INFO) {
		color = FOREGROUND_GREEN;
	} else if (_logConsoleColors && priority == SDL_LOG_PRIORITY_WARN) {
		color = FOREGROUND_GREEN | FOREGROUND_RED;
	} else if (_logConsoleColors && (priority == SDL_LOG_PRIORITY_ERROR || priority == SDL_LOG_PRIORITY_CRITICAL)) {
		color = FOREGROUND_RED;
	}

	if (color == -1) {
		_logCallback(_logCallbackUserData, category, priority, message);
		return;
	}

	HANDLE terminalHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	// query current settings
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(terminalHandle, &info)) {
		_logCallback(_logCallbackUserData, category, priority, message);
		return;
	}
	info.wAttributes &= ~(info.wAttributes & 0x0F);
	info.wAttributes |= (WORD)color;

	SetConsoleTextAttribute(terminalHandle, info.wAttributes);
	_logCallback(_logCallbackUserData, category, priority, message);
	SetConsoleTextAttribute(terminalHandle, defaultAttributes);
#else
	_logCallback(_logCallbackUserData, category, priority, message);
#endif
}

} // priv

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

void Log::registerLogListener(ILogListener *listener) {
	if (listener == nullptr) {
		return;
	}
	priv::_logListener = listener;
}

void Log::unregisterLogListener(ILogListener *listener) {
	if (priv::_logListener == listener) {
		priv::_logListener = nullptr;
	}
}

void Log::setConsoleColors(bool enabled) {
	priv::_logConsoleColors = enabled;
}

void Log::setLevel(Level level) {
	priv::_logLevel = level;
}

void Log::init(const char *logfile) {
	int rawLogLevel = core::getVar(cfg::CoreLogLevel)->intVal();
	if (rawLogLevel < 0 || rawLogLevel > (int)Log::Level::Error) {
		rawLogLevel = (int)Log::Level::Error;
	}
	priv::_logLevel = (Log::Level)rawLogLevel;
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, (SDL_LogPriority)Log::Level::Trace);

	if (priv::_logfile == nullptr && logfile != nullptr) {
		priv::_logfile = fopen(logfile, "w");
	}

#ifdef _WIN32
	HANDLE terminalHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo(terminalHandle, &info)) {
		priv::defaultAttributes = info.wAttributes;
	}
#endif

	if (priv::_logCallback == nullptr) {
		SDL_LogGetOutputFunction(&priv::_logCallback, &priv::_logCallbackUserData);
		SDL_LogSetOutputFunction(priv::logOutputFunction, nullptr);
	}
}

void Log::shutdown() {
	// this is one of the last methods that is executed - so don't rely on anything
	// still being available here - it won't
	if (priv::_logCallback != nullptr) {
		SDL_LogSetOutputFunction(priv::_logCallback, priv::_logCallbackUserData);
		priv::_logCallback = nullptr;
		priv::_logCallbackUserData = nullptr;
	}
	if (priv::_logfile) {
		fflush(priv::_logfile);
		fclose(priv::_logfile);
		priv::_logfile = nullptr;
	}
	priv::_logListener = nullptr;
	priv::_logLevel = Log::Level::Info;
}

static void notifyLogListeners(Log::Level level, const char *buf) {
	if (priv::_logListener) {
		priv::_logListener->onLog(level, buf);
	}
}

static void traceVA(const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[TRACE] %s\n", buf);
	}
	notifyLogListeners(Log::Level::Trace, buf);
	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
}

static void debugVA(const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[DEBUG] %s\n", buf);
	}
	notifyLogListeners(Log::Level::Debug, buf);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
}

static void infoVA(const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[INFO] %s\n", buf);
	}
	notifyLogListeners(Log::Level::Info, buf);
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
}

static void warnVA(const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[WARN] %s\n", buf);
	}
	notifyLogListeners(Log::Level::Warn, buf);
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
}

static void errorVA(const char *msg, va_list args) {
	char buf[priv::bufSize];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	if (priv::_logfile) {
		fprintf(priv::_logfile, "[ERROR] %s\n", buf);
	}
	notifyLogListeners(Log::Level::Error, buf);
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", buf);
}

static void printfVA(const char *msg, va_list args) {
	char tempBuf[priv::bufSize];

	va_list args_copy;
	va_copy(args_copy, args);
	int neededSize = SDL_vsnprintf(tempBuf, sizeof(tempBuf), msg, args_copy);
	va_end(args_copy);

	if (neededSize < (int)sizeof(tempBuf)) {
		// Output fits in tempBuf
		if (priv::_logfile) {
			fprintf(priv::_logfile, "%s", tempBuf);
		}
		printf("%s", tempBuf);
	} else {
		// Allocate a large enough buffer
		char *buf = (char *)core_malloc(neededSize + 1);
		if (!buf)
			return; // Handle memory allocation failure

		va_copy(args_copy, args);
		SDL_vsnprintf(buf, neededSize + 1, msg, args_copy);
		va_end(args_copy);

		if (priv::_logfile) {
			fprintf(priv::_logfile, "%s", buf);
		}
		printf("%s", buf);
		core_free(buf);
	}
}

void Log::trace(const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Trace) {
		return;
	}
	va_list args;
	va_start(args, msg);
	traceVA(msg, args);
	va_end(args);
}

void Log::debug(const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Debug) {
		return;
	}
	va_list args;
	va_start(args, msg);
	debugVA(msg, args);
	va_end(args);
}

void Log::info(const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Info) {
		return;
	}
	va_list args;
	va_start(args, msg);
	infoVA(msg, args);
	va_end(args);
}

void Log::warn(const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Warn) {
		return;
	}
	va_list args;
	va_start(args, msg);
	warnVA(msg, args);
	va_end(args);
}

void Log::error(const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Error) {
		return;
	}
	va_list args;
	va_start(args, msg);
	errorVA(msg, args);
	va_end(args);
}

void Log::printf(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	printfVA(msg, args);
	va_end(args);
}

void c_logtrace(const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Trace) {
		return;
	}
	va_list args;
	va_start(args, msg);
	traceVA(msg, args);
	va_end(args);
}

void c_logdebug(const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Debug) {
		return;
	}
	va_list args;
	va_start(args, msg);
	debugVA(msg, args);
	va_end(args);
}

void c_loginfo(const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Info) {
		return;
	}
	va_list args;
	va_start(args, msg);
	infoVA(msg, args);
	va_end(args);
}

void c_logwarn(const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Warn) {
		return;
	}
	va_list args;
	va_start(args, msg);
	warnVA(msg, args);
	va_end(args);
}

void c_logerror(CORE_FORMAT_STRING const char* msg, ...) {
	if (priv::_logLevel > Log::Level::Error) {
		return;
	}
	va_list args;
	va_start(args, msg);
	errorVA(msg, args);
	va_end(args);
}

extern "C" void c_logwrite(const char* msg, size_t length) {
	char buf[priv::bufSize];
	SDL_memcpy(buf, msg, core_max(priv::bufSize - 1, length));
	buf[sizeof(buf) - 1] = '\0';
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", buf);
}
