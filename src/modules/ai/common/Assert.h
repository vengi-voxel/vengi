/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include <assert.h>

#define ai_assert core_assert_msg

/**
 * @brief Provide your own assert - this is also executed in non DEBUG mode
 */
#ifndef ai_assert_always
	#ifdef __clang_analyzer__
		#define ai_assert_always(condition, ...) assert(condition)
	#else
		#define ai_assert_always(condition, ...) \
			if ( !(condition) ) { \
				ai_log_error(__VA_ARGS__); \
				ai_log_error("%s:%i", __FILE__, __LINE__); \
				assert(condition); \
			}
	#endif
#endif
