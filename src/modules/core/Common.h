/**
 * @file
 */

#pragma once

#define CORE_STRINGIFY_INTERNAL(x) #x
#define CORE_STRINGIFY(x) CORE_STRINGIFY_INTERNAL(x)

#ifndef __GNUC__
#define __attribute__(x)
#endif

#if defined(_MSC_VER)
#define CORE_FORCE_INLINE __forceinline
#elif ( (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__) )
#define CORE_FORCE_INLINE __attribute__((always_inline)) static __inline__
#else
#define CORE_FORCE_INLINE static inline
#endif

#ifdef _MSC_VER
#include <sal.h>
#if _MSC_VER >= 1400
#define CORE_FORMAT_STRING _Printf_format_string_
#else
#define CORE_FORMAT_STRING __format_string
#endif
#else
#define CORE_FORMAT_STRING
#endif
#if defined(__GNUC__)
#define CORE_PRINTF_VARARG_FUNC(fmtargnumber) __attribute__((format(__printf__, fmtargnumber, fmtargnumber + 1)))
#define CORE_SCANF_VARARG_FUNC(fmtargnumber) __attribute__((format(__scanf__, fmtargnumber, fmtargnumber + 1)))
#else
#define CORE_PRINTF_VARARG_FUNC(fmtargnumber)
#define CORE_SCANF_VARARG_FUNC(fmtargnumber)
#endif

#if (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800)) || defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 3))
#define CORE_EXPECT(expr, value) (__builtin_expect((expr), (value)))
#else
#define CORE_EXPECT(expr, value) (expr)
#endif
#define core_likely(expr) CORE_EXPECT((expr) != 0, 1)
#define core_unlikely(expr) CORE_EXPECT((expr) != 0, 0)

#define core_min(x, y) ((x) < (y) ? (x) : (y))
#define core_max(x, y) ((x) > (y) ? (x) : (y))

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

namespace core {

template<class T>
struct remove_reference { typedef T type; };
template<class T>
struct remove_reference<T &> { typedef T type; };
template<class T>
struct remove_reference<T &&> { typedef T type; };

template<class T>
constexpr typename remove_reference<T>::type &&move(T &&t) {
	return ((typename remove_reference<T>::type &&) t);
}

template<typename T>
constexpr T &&forward(typename remove_reference<T>::type &t) noexcept {
	return static_cast<T &&>(t);
}

template<typename T>
constexpr T &&forward(typename remove_reference<T>::type &&t) {
	return static_cast<T &&>(t);
}

template<typename T>
struct remove_const {
	typedef T type;
};

}
