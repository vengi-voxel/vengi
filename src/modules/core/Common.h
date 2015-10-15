#pragma once

#include "Log.h"
#include <cassert>
#include <cmath>
//#include <SDL_assert.h>

namespace core {

inline float toRadians(float degree) {
	return degree * static_cast<float>(M_PI) / 180.0f;
}

inline float toDegrees(float radians) {
	return radians * 180.0f / static_cast<float>(M_PI);
}

template<typename T>
inline T clamp(T a, T low, T high) {
	return std::max(low, std::min(a, high));
}

}

#define lengthof(x) (sizeof(x) / sizeof(*(x)))
/** @brief compile time assert */
#define CASSERT(x) extern int ASSERT_COMPILE[((x) != 0) * 2 - 1]

#define CORE_STRINGIFY_INTERNAL(x) #x
#define CORE_STRINGIFY(x) CORE_STRINGIFY_INTERNAL(x)

#ifndef core_assert
//#define core_assert(condition) SDL_assert(condition)
#define core_assert(condition) assert(condition)
#endif
