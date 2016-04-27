#pragma once

#include "Log.h"
#include <string>
#include <unordered_map>
#include <cassert>
#include <iostream>
#include <cstdio>

#ifndef ai_log
#define ai_log(format, ...) ai::Log::info(format, ##__VA_ARGS__)
#endif

#ifndef ai_log_error
#define ai_log_error(format, ...) ai::Log::error(format, ##__VA_ARGS__)
#endif

#ifndef ai_log_warn
#define ai_log_warn(format, ...) ai::Log::warn(format, ##__VA_ARGS__)
#endif

#ifndef ai_log_debug
#define ai_log_debug(format, ...) ai::Log::debug(format, ##__VA_ARGS__)
#endif

#ifndef ai_log_trace
#define ai_log_trace(format, ...) ai::Log::trace(format, ##__VA_ARGS__)
#endif

#if !(__GNUC__ || __GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#ifndef ai_assert
	#ifdef DEBUG
		#if (__clang_analyzer__)
			#define ai_assert(condition, format, ...) assert(condition)
		#else
			#define ai_assert(condition, format, ...) \
				if ( !(condition) ) { \
					ai::Log::error(format, ##__VA_ARGS__); \
					ai::Log::error("%s:%i", __FILE__, __LINE__); \
					assert(condition); \
				}
		#endif
	#else
		#define ai_assert(condition, format, ...)
	#endif
#endif

template<class T, class S>
inline T ai_assert_cast(const S object) {
#ifdef __cpp_rtti
	ai_assert(dynamic_cast<T>(object) == static_cast<T>(object), "Types don't match");
#endif
	return static_cast<T>(object);
}

#define AI_STRINGIFY_INTERNAL(x) #x
#define AI_STRINGIFY(x) AI_STRINGIFY_INTERNAL(x)

#ifndef AI_EXCEPTIONS
#define AI_EXCEPTIONS 0
#endif

#ifdef _WIN32
#	ifdef SIMPLEAI_EXPORT
#		define SIMPLEAI_LIB __declspec(dllexport)
#	elif defined(SIMPLEAI_IMPORT)
#		define SIMPLEAI_LIB __declspec(dllimport)
#	else
#		define SIMPLEAI_LIB
#	endif
#else
#	define SIMPLEAI_LIB
#endif

namespace ai {

/**
 * @brief Defines the type of the id to identify an ai controlled entity.
 *
 * @note @c -1 is reserved. You should use ids >= 0
 * @sa NOTHING_SELECTED
 */
typedef int CharacterId;

typedef std::unordered_map<std::string, std::string> CharacterAttributes;

}
