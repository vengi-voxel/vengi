/**
 * @file
 */

#pragma once

#include <SDL.h>
#ifndef __WINDOWS__
#define HAVE_BACKWARD
#endif

#ifdef HAVE_BACKWARD
#include <backward.h>
#define core_stacktrace \
	backward::StackTrace st; \
	st.load_here(32); \
	backward::TraceResolver tr; \
	tr.load_stacktrace(st); \
	for (size_t __stacktrace_i = 0; __stacktrace_i < st.size(); ++__stacktrace_i) { \
		const backward::ResolvedTrace& trace = tr.resolve(st[__stacktrace_i]); \
		printf("#%i %s %s [%p]\n", int(__stacktrace_i), trace.object_filename.c_str(), trace.object_function.c_str(), trace.addr); \
	}
#else
#define core_stacktrace
#endif

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
			static char __assertBuf[1024]; \
			SDL_snprintf(__assertBuf, sizeof(__assertBuf) - 1, format, ##__VA_ARGS__); \
			static struct SDL_AssertData sdl_assert_data = { \
				0, 0, nullptr, 0, 0, 0, 0 \
			}; \
			sdl_assert_data.condition = __assertBuf; /* also let it work for following calls */ \
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

#define core_assert_2byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 1) == 0, "Data is not aligned properly");
#define core_assert_4byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 3) == 0, "Data is not aligned properly");
#define core_assert_8byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 7) == 0, "Data is not aligned properly");
#define core_assert_16byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 15) == 0, "Data is not aligned properly");
#define core_assert_32byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 31) == 0, "Data is not aligned properly");
#define core_assert_64byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 63) == 0, "Data is not aligned properly");
#define core_assert_128byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 127) == 0, "Data is not aligned properly");

template<class T, class S>
inline T assert_cast(const S object) {
#ifdef __cpp_rtti
	core_assert(dynamic_cast<T>(object) == static_cast<T>(object));
#endif
	return static_cast<T>(object);
}
