/**
 * @file
 * @mainpage VoxelEngine documentation
 *
 * - [GitLab page](http://gitlab.com/mgerhardy/engine/)
 */

#pragma once

#include "Log.h"
#include <math.h>
#include <utility>
#include <limits>
#include <SDL.h>

#define CORE_STRINGIFY_INTERNAL(x) #x
#define CORE_STRINGIFY(x) CORE_STRINGIFY_INTERNAL(x)

#define CORE_CLASS(name) \
	friend class name##Test;

#define CORE_ENUM_BIT_OPERATIONS(EnumClassName) \
	inline constexpr EnumClassName operator&(EnumClassName __x, EnumClassName __y) { \
		return static_cast<EnumClassName>(static_cast<int>(__x) & static_cast<int>(__y)); \
	} \
	inline constexpr EnumClassName operator|(EnumClassName __x, EnumClassName __y) { \
		return static_cast<EnumClassName>(static_cast<int>(__x) | static_cast<int>(__y)); \
	} \
	inline constexpr EnumClassName operator^(EnumClassName __x, EnumClassName __y) { \
		return static_cast<EnumClassName>(static_cast<int>(__x) ^ static_cast<int>(__y)); \
	} \
	inline constexpr EnumClassName operator~(EnumClassName __x) { \
		return static_cast<EnumClassName>(~static_cast<int>(__x)); \
	} \
	inline EnumClassName& operator&=(EnumClassName & __x, EnumClassName __y) { \
		__x = __x & __y; \
		return __x; \
	} \
	inline EnumClassName& operator|=(EnumClassName & __x, EnumClassName __y) { \
		__x = __x | __y; \
		return __x; \
	} \
	inline EnumClassName& operator^=(EnumClassName & __x, EnumClassName __y) { \
		__x = __x ^ __y; \
		return __x; \
	}


#ifndef core_malloc
#define core_malloc SDL_malloc
#endif

#ifndef core_realloc
#define core_realloc SDL_realloc
#endif

#ifndef core_free
#define core_free SDL_free
#endif

#ifndef core_memset
#define core_memset SDL_memset
#endif

#ifndef core_zero
#define core_zero SDL_zero
#endif

#ifndef core_zerop
#define core_zerop SDL_zerop
#endif

#define MAKE_SHARED_INVIS_CTOR(classname) \
	struct make_shared_enabler: public classname { \
		template<typename ... Args> \
		make_shared_enabler(Args&&... args) : \
				classname(std::forward<Args>(args)...) { \
		} \
	}

namespace std {

template<typename T>
constexpr typename underlying_type<T>::type enum_value(const T& val) {
	return static_cast<typename underlying_type<T>::type>(val);
}
}

struct EnumClassHash {
template<typename T>
auto operator()(T t) const {
	return std::enum_value(t);
}
};

inline constexpr uint32_t FourCC(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
	return ((uint32_t) ((uint32_t(d) << 24) | (uint32_t(c) << 16) | (uint32_t(b) << 8) | uint32_t(a)));
}

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
