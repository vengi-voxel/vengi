/**
 * @file
 */

#pragma once

#include "Common.h"
#include <SDL_assert.h>

extern void core_stacktrace();

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
			if (sdl_assert_data.always_ignore == 0) { \
				core_stacktrace(); \
			} \
			const SDL_AssertState sdl_assert_state = SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE); \
			if (sdl_assert_state == SDL_ASSERTION_RETRY) { \
				continue; /* go again. */ \
			} \
			if (sdl_assert_state == SDL_ASSERTION_BREAK) { \
				SDL_TriggerBreakpoint(); \
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

extern SDL_AssertState core_assert_impl_message(SDL_AssertData &sdl_assert_data, char *buf, int bufSize,
										const char *function, const char *file, int line, CORE_FORMAT_STRING const char *format, ...)
										CORE_PRINTF_VARARG_FUNC(7);

#define core_assert_msg(conditionCheck, format, ...) \
	do { \
		while (!(conditionCheck)) { \
			static struct SDL_AssertData sdl_assert_data = { \
				0, 0, nullptr, 0, 0, 0, 0 \
			}; \
			if (sdl_assert_data.always_ignore == 0) { \
				core_stacktrace(); \
			} \
			static char __assertBuf[1024]; \
			const SDL_AssertState sdl_assert_state = core_assert_impl_message(sdl_assert_data, __assertBuf, \
					sizeof(__assertBuf), SDL_FUNCTION, SDL_FILE, SDL_LINE, format, ##__VA_ARGS__); \
			if (sdl_assert_state == SDL_ASSERTION_RETRY) { \
				continue; /* go again. */ \
			} \
			break; /* not retrying. */ \
		} \
	} while (SDL_NULL_WHILE_LOOP_CONDITION)
#endif
#endif

#ifdef _MSC_VER
#define core_assert_2byte_aligned(data)
#define core_assert_4byte_aligned(data)
#define core_assert_8byte_aligned(data)
#define core_assert_16byte_aligned(data)
#define core_assert_32byte_aligned(data)
#define core_assert_64byte_aligned(data)
#define core_assert_128byte_aligned(data)
#else
#define core_assert_2byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 1) == 0, "Data is not aligned properly");
#define core_assert_4byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 3) == 0, "Data is not aligned properly");
#define core_assert_8byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 7) == 0, "Data is not aligned properly");
#define core_assert_16byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 15) == 0, "Data is not aligned properly");
#define core_assert_32byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 31) == 0, "Data is not aligned properly");
#define core_assert_64byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 63) == 0, "Data is not aligned properly");
#define core_assert_128byte_aligned(data) core_assert_msg((((uintptr_t )(data)) & 127) == 0, "Data is not aligned properly");
#endif
