#pragma once

#include <SDL.h>
#include <backward.h>

#define core_stacktrace \
	backward::StackTrace st; \
	st.load_here(32); \
	backward::TraceResolver tr; \
	tr.load_stacktrace(st); \
	for (size_t i = 0; i < st.size(); ++i) { \
		const backward::ResolvedTrace& trace = tr.resolve(st[i]); \
		printf("#%i %s %s [%p]\n", int(i), trace.object_filename.c_str(), trace.object_function.c_str(), trace.addr); \
	}

#ifndef core_assert
#if SDL_ASSERT_LEVEL <= 0
#define core_assert(condition) SDL_disabled_assert(condition)
#else
#define core_assert(condition) \
	do { \
		while ( !(condition) ) { \
			static struct SDL_AssertData sdl_assert_data = { \
				0, 0, #condition, 0, 0, 0, 0 \
			}; \
			const SDL_AssertState sdl_assert_state = SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE); \
			if (sdl_assert_state == SDL_ASSERTION_RETRY) { \
				continue; /* go again. */ \
			} else if (sdl_assert_state == SDL_ASSERTION_BREAK) { \
				SDL_TriggerBreakpoint(); \
			} else if (sdl_assert_state != SDL_ASSERTION_ALWAYS_IGNORE) { \
				core_stacktrace \
			} \
			break; /* not retrying. */ \
		} \
	} while (SDL_NULL_WHILE_LOOP_CONDITION)
#endif
#endif

#define core_assert_always SDL_assert_always

#ifndef core_assert_msg
#if SDL_ASSERT_LEVEL <= 0
#define core_assert_msg(condition, format, ...) SDL_disabled_assert(condition)
#else
#define core_assert_msg(conditionCheck, format, ...) \
	do { \
		while (!(conditionCheck)) { \
			static char buf[1024]; \
			SDL_snprintf(buf, sizeof(buf) - 1, format, ##__VA_ARGS__); \
			static struct SDL_AssertData sdl_assert_data = { \
				0, 0, nullptr, 0, 0, 0, 0 \
			}; \
			sdl_assert_data.condition = buf; /* also let it work for following calls */ \
			const SDL_AssertState sdl_assert_state = SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE); \
			if (sdl_assert_state == SDL_ASSERTION_RETRY) { \
				continue; /* go again. */ \
			} else if (sdl_assert_state == SDL_ASSERTION_BREAK) { \
				SDL_TriggerBreakpoint(); \
			} else if (sdl_assert_state != SDL_ASSERTION_ALWAYS_IGNORE) { \
				core_stacktrace \
			} \
			break; /* not retrying. */ \
		} \
	} while (SDL_NULL_WHILE_LOOP_CONDITION)
#endif
#endif

template<class T, class S>
inline T assert_cast(const S object) {
#ifdef __cpp_rtti
	core_assert(dynamic_cast<T>(object) == static_cast<T>(object));
#endif
	return static_cast<T>(object);
}
