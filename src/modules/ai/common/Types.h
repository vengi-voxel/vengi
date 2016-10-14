/**
 * @file
 */
#pragma once

#include "Log.h"
#include <string>
#include <unordered_map>
#include <cassert>
#include <cstdio>

/**
 * @brief Logging macro to provide your own loggers
 */
#ifndef ai_log
#define ai_log(...) ai::Log::info(__VA_ARGS__)
#endif

/**
 * @brief Logging macro to provide your own loggers
 */
#ifndef ai_log_error
#define ai_log_error(...) ai::Log::error(__VA_ARGS__)
#endif

/**
 * @brief Logging macro to provide your own loggers
 */
#ifndef ai_log_warn
#define ai_log_warn(...) ai::Log::warn(__VA_ARGS__)
#endif

/**
 * @brief Logging macro to provide your own loggers
 */
#ifndef ai_log_debug
#define ai_log_debug(...) ai::Log::debug(__VA_ARGS__)
#endif

/**
 * @brief Logging macro to provide your own loggers
 */
#ifndef ai_log_trace
#define ai_log_trace(...) ai::Log::trace(__VA_ARGS__)
#endif

#if !(__GNUC__ || __GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/**
 * @brief Provide your own assert - this is also executed in non DEBUG mode
 */
#ifndef ai_assert_always
	#if (__clang_analyzer__)
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

/**
 * @brief Provide your own assert - this is only executed in DEBUG mode
 * @see ai_assert_always
 */
#ifndef ai_assert
	#ifdef DEBUG
		#define ai_assert ai_assert_always
	#else
		#define ai_assert(condition, ...)
	#endif
#endif

/**
 * @brief If you compile with RTTI activated, you get additional sanity checks when using this
 * macro to perform your @c static_cast calls
 */
template<class T, class S>
inline T ai_assert_cast(const S object) {
#ifdef __cpp_rtti
	ai_assert(dynamic_cast<T>(object) == static_cast<T>(object), "Types don't match");
#endif
	return static_cast<T>(object);
}

#define AI_STRINGIFY_INTERNAL(x) #x
#define AI_STRINGIFY(x) AI_STRINGIFY_INTERNAL(x)

/**
 * @brief If you want to use exceptions in your code and want them to be catched by the library
 * just set this to 1
 */
#ifndef AI_EXCEPTIONS
#define AI_EXCEPTIONS 0
#endif

/**
 * @brief Enable lua sanity checks by default
 */
#ifndef AI_LUA_SANTITY
#define AI_LUA_SANTITY 1
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

#define DIAG_STR(s) #s
#define DIAG_JOINSTR(x,y) DIAG_STR(x ## y)
#ifdef _MSC_VER
#define DIAG_DO_PRAGMA(x) __pragma (#x)
#define DIAG_PRAGMA(compiler,x) DIAG_DO_PRAGMA(warning(x))
#else
#define DIAG_DO_PRAGMA(x) _Pragma (#x)
#define DIAG_PRAGMA(compiler,x) DIAG_DO_PRAGMA(compiler diagnostic x)
#endif
#if defined(__clang__)
# define DISABLE_WARNING(gcc_unused,clang_option,msvc_unused) DIAG_PRAGMA(clang,push) DIAG_PRAGMA(clang,ignored DIAG_JOINSTR(-W,clang_option))
# define ENABLE_WARNING(gcc_unused,clang_option,msvc_unused) DIAG_PRAGMA(clang,pop)
#elif defined(_MSC_VER)
# define DISABLE_WARNING(gcc_unused,clang_unused,msvc_errorcode) DIAG_PRAGMA(msvc,push) DIAG_DO_PRAGMA(warning(disable:##msvc_errorcode))
# define ENABLE_WARNING(gcc_unused,clang_unused,msvc_errorcode) DIAG_PRAGMA(msvc,pop)
#elif defined(__GNUC__)
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
# define DISABLE_WARNING(gcc_option,clang_unused,msvc_unused) DIAG_PRAGMA(GCC,push) DIAG_PRAGMA(GCC,ignored DIAG_JOINSTR(-W,gcc_option))
# define ENABLE_WARNING(gcc_option,clang_unused,msvc_unused) DIAG_PRAGMA(GCC,pop)
#else
# define DISABLE_WARNING(gcc_option,clang_unused,msvc_unused) DIAG_PRAGMA(GCC,ignored DIAG_JOINSTR(-W,gcc_option))
# define ENABLE_WARNING(gcc_option,clang_option,msvc_unused) DIAG_PRAGMA(GCC,warning DIAG_JOINSTR(-W,gcc_option))
#endif
#endif

namespace ai {

/**
 * @brief Defines the type of the id to identify an ai controlled entity.
 *
 * @note @c -1 is reserved. You should use ids >= 0
 * @sa NOTHING_SELECTED
 */
typedef int CharacterId;

/**
 * @brief ICharacter attributes for the remote \ref debugger
 */
typedef std::unordered_map<std::string, std::string> CharacterAttributes;

}
