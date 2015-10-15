#include "Log.h"
#include "Var.h"
#include <cstring>
#include <SDL.h>

enum {
	TRACE = 4, DEBUG = 3, INFO = 2, WARN = 1, ERROR = 0
};

Log::Log() {
	_logLevel = core::Var::get("core_loglevel", "2");
}

void Log::vsnprint(const char* msg, va_list args) {
	char buf[1024];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = 0;
	if (buf[strlen(buf) - 1] == '\r')
		SDL_Log("%s\n", buf);
	else
		SDL_Log("%s\n", buf);
}

void Log::trace(const char* msg, ...) {
	if (get()._logLevel->intVal() < DEBUG)
		return;
	va_list args;
	va_start(args, msg);
	get().vsnprint(msg, args);
	va_end(args);
}

void Log::debug(const char* msg, ...) {
	if (get()._logLevel->intVal() < DEBUG)
		return;
	va_list args;
	va_start(args, msg);
	get().vsnprint(msg, args);
	va_end(args);
}

void Log::info(const char* msg, ...) {
	if (get()._logLevel->intVal() < INFO)
		return;
	va_list args;
	va_start(args, msg);
	get().vsnprint(msg, args);
	va_end(args);
}

void Log::warn(const char* msg, ...) {
	if (get()._logLevel->intVal() < INFO)
		return;
	va_list args;
	va_start(args, msg);
	get().vsnprint(msg, args);
	va_end(args);
}

void Log::error(const char* msg, ...) {
	if (get()._logLevel->intVal() < INFO)
		return;
	va_list args;
	va_start(args, msg);
	get().vsnprint(msg, args);
	va_end(args);
}
