/**
 * @file
 */

#include "Assert.h"
#include "Log.h"
#include <SDL_assert.h>

#ifdef HAVE_BACKWARD
#include <backward.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static_assert(sizeof(SDL_AssertData) == sizeof(AssertData));

bool core_report_assert(AssertData &data, const char *file, int line, const char *function) {
	if (data.always_ignore == 0) {
		core_stacktrace();
	}
	const SDL_AssertState state = SDL_ReportAssertion((SDL_AssertData *)&data, function, file, line);
	if (state == SDL_ASSERTION_RETRY) {
		return true;
	}
	if (state == SDL_ASSERTION_BREAK) {
		SDL_TriggerBreakpoint();
	}
	return false;
}

void core_stacktrace() {
#ifdef HAVE_BACKWARD
	std::ostringstream os;
	backward::StackTrace st;
	st.load_here(32);
	backward::Printer printer;
	printer.print(st, os);
	std::string stacktrace = os.str();
	char *c = (char *)stacktrace.data();
	while (char *l = SDL_strchr(c, '\n')) {
		*l = '\0';
		Log::error("%s", c);
		c = l + 1;
	}
#endif
#ifdef __EMSCRIPTEN__
	EM_ASM({ stackTrace(); });
#endif
}

bool core_assert_impl_message(AssertData &data, char *buf, int bufSize, const char *function, const char *file,
							  int line, const char *format, ...) {
	va_list args;
	va_start(args, format);
	SDL_vsnprintf(buf, bufSize - 1, format, args);
	va_end(args);
	data.condition = buf; /* also let it work for following calls */
	const SDL_AssertState state = SDL_ReportAssertion((SDL_AssertData *)&data, function, file, line);
	if (state == SDL_ASSERTION_RETRY) {
		return true;
	}
	if (state == SDL_ASSERTION_BREAK) {
		SDL_TriggerBreakpoint();
	} else if (data.trigger_count <= 1) {
		core_stacktrace();
	}
	return false;
}
