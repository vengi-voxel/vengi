/**
 * @file
 */

#pragma once

#include "core/Common.h"

struct AssertData {
	int always_ignore;
	unsigned int trigger_count;
	const char *condition;
	const char *filename;
	int linenum;
	const char *function;
	struct AssertData *next;
};

void core_stacktrace();
bool core_report_assert(AssertData &data, const char *file, int line, const char *function);

/* "while (0,0)" fools Microsoft's compiler's /W4 warning level into thinking
	this condition isn't constant. And looks like an owl's face! */
#ifdef _MSC_VER /* stupid /W4 warnings. */
#define CORE_NULL_WHILE_LOOP_CONDITION (0, 0)
#else
#define CORE_NULL_WHILE_LOOP_CONDITION (0)
#endif

#ifdef NDEBUG

#define core_assert(condition) do { } while (CORE_NULL_WHILE_LOOP_CONDITION)
#define core_assert_msg(conditionCheck, ...) do { } while (CORE_NULL_WHILE_LOOP_CONDITION)

#else
#define core_assert(condition)                                                                                         \
	do {                                                                                                               \
		while (!(condition)) {                                                                                         \
			static struct AssertData assert_data = {0, 0, #condition, 0, 0, 0, 0};                                     \
			if (core_report_assert(assert_data, CORE_FILE, CORE_LINE, CORE_FUNCTION)) {                                \
				continue;                                                                                              \
			}                                                                                                          \
			break; /* not retrying. */                                                                                 \
		}                                                                                                              \
	} while (CORE_NULL_WHILE_LOOP_CONDITION)

bool core_assert_impl_message(AssertData &assert_data, char *buf, int bufSize, const char *function, const char *file,
							  int line, CORE_FORMAT_STRING const char *format, ...) CORE_PRINTF_VARARG_FUNC(7);

#define core_assert_msg(conditionCheck, ...)                                                                           \
	do {                                                                                                               \
		while (!(conditionCheck)) {                                                                                    \
			static struct AssertData assert_data = {0, 0, nullptr, 0, 0, 0, 0};                                        \
			if (assert_data.always_ignore == 0) {                                                                      \
				core_stacktrace();                                                                                     \
			}                                                                                                          \
			static char __assertBuf[1024];                                                                             \
			if (core_assert_impl_message(assert_data, __assertBuf, sizeof(__assertBuf), CORE_FUNCTION, CORE_FILE,      \
										 CORE_LINE, ##__VA_ARGS__)) {                                                  \
				continue; /* go again. */                                                                              \
			}                                                                                                          \
			break; /* not retrying. */                                                                                 \
		}                                                                                                              \
	} while (CORE_NULL_WHILE_LOOP_CONDITION)
#endif

#define core_assert_always(condition)                                                                                         \
	do {                                                                                                               \
		while (!(condition)) {                                                                                         \
			static struct AssertData assert_data = {0, 0, #condition, 0, 0, 0, 0};                                     \
			if (core_report_assert(assert_data, CORE_FILE, CORE_LINE, CORE_FUNCTION)) {                                \
				continue;                                                                                              \
			}                                                                                                          \
			break; /* not retrying. */                                                                                 \
		}                                                                                                              \
	} while (CORE_NULL_WHILE_LOOP_CONDITION)

bool core_assert_impl_message(AssertData &assert_data, char *buf, int bufSize, const char *function, const char *file,
							  int line, CORE_FORMAT_STRING const char *format, ...) CORE_PRINTF_VARARG_FUNC(7);

#define core_assert_msg_always(conditionCheck, ...)                                                                           \
	do {                                                                                                               \
		while (!(conditionCheck)) {                                                                                    \
			static struct AssertData assert_data = {0, 0, nullptr, 0, 0, 0, 0};                                        \
			if (assert_data.always_ignore == 0) {                                                                      \
				core_stacktrace();                                                                                     \
			}                                                                                                          \
			static char __assertBuf[1024];                                                                             \
			if (core_assert_impl_message(assert_data, __assertBuf, sizeof(__assertBuf), CORE_FUNCTION, CORE_FILE,      \
										 CORE_LINE, ##__VA_ARGS__)) {                                                  \
				continue; /* go again. */                                                                              \
			}                                                                                                          \
			break; /* not retrying. */                                                                                 \
		}                                                                                                              \
	} while (CORE_NULL_WHILE_LOOP_CONDITION)

#ifdef _MSC_VER
#define core_assert_2byte_aligned(data)
#define core_assert_4byte_aligned(data)
#define core_assert_8byte_aligned(data)
#define core_assert_16byte_aligned(data)
#define core_assert_32byte_aligned(data)
#define core_assert_64byte_aligned(data)
#define core_assert_128byte_aligned(data)
#else
#define core_assert_2byte_aligned(data) core_assert_msg((((uintptr_t)(data)) & 1) == 0, "Data is not aligned properly");
#define core_assert_4byte_aligned(data) core_assert_msg((((uintptr_t)(data)) & 3) == 0, "Data is not aligned properly");
#define core_assert_8byte_aligned(data) core_assert_msg((((uintptr_t)(data)) & 7) == 0, "Data is not aligned properly");
#define core_assert_16byte_aligned(data)                                                                               \
	core_assert_msg((((uintptr_t)(data)) & 15) == 0, "Data is not aligned properly");
#define core_assert_32byte_aligned(data)                                                                               \
	core_assert_msg((((uintptr_t)(data)) & 31) == 0, "Data is not aligned properly");
#define core_assert_64byte_aligned(data)                                                                               \
	core_assert_msg((((uintptr_t)(data)) & 63) == 0, "Data is not aligned properly");
#define core_assert_128byte_aligned(data)                                                                              \
	core_assert_msg((((uintptr_t)(data)) & 127) == 0, "Data is not aligned properly");
#endif
