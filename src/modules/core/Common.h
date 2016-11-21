/**
 * @file
 * @mainpage VoxelEngine documentation
 *
 * - [GitLab page](http://gitlab.com/mgerhardy/engine/)
 */

#pragma once

#include "GLM.h"
#include "Log.h"
#include <cmath>
#include <utility>
#include <limits>
#include <backward.h>
#include <SDL.h>
#include <SDL_assert.h>

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
			} \
			core_stacktrace \
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
			} \
			core_stacktrace \
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

#define MAKE_SHARED_INVIS_CTOR(classname) \
	struct make_shared_enabler: public classname { \
		template<typename ... Args> \
		make_shared_enabler(Args&&... args) : \
				classname(std::forward<Args>(args)...) { \
		} \
	}

namespace std {
template<>
struct hash<glm::ivec3> {
	std::size_t operator()(const glm::ivec3& vec) const {
		return ((vec.x & 0xFF)) | ((vec.y & 0xFF) << 8) | ((vec.z & 0xFF) << 16);
	}
};

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

#include <vector>

namespace core {

template<class T>
inline size_t vectorSize(const std::vector<T>& v) {
	return v.size() * sizeof(T);
}

}
