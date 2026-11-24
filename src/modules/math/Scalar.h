/**
 * @file
 */

#pragma once

#include <stdint.h>
#include "math/Constants.h"

// libm prototypes. Declared here so this header does not include <math.h>.
// glibc marks these noexcept; a bare prototype fails when the same TU also includes <cmath>.
#ifdef __GLIBC__
#define MATH_LIBM_NOEXCEPT noexcept
#else
#define MATH_LIBM_NOEXCEPT
#endif
extern "C" {
float floorf(float) MATH_LIBM_NOEXCEPT;
float ceilf(float) MATH_LIBM_NOEXCEPT;
float roundf(float) MATH_LIBM_NOEXCEPT;
float sqrtf(float) MATH_LIBM_NOEXCEPT;
float powf(float, float) MATH_LIBM_NOEXCEPT;
float expf(float) MATH_LIBM_NOEXCEPT;
float exp2f(float) MATH_LIBM_NOEXCEPT;
float ldexpf(float, int) MATH_LIBM_NOEXCEPT;
float logf(float) MATH_LIBM_NOEXCEPT;
float log2f(float) MATH_LIBM_NOEXCEPT;
float sinf(float) MATH_LIBM_NOEXCEPT;
float cosf(float) MATH_LIBM_NOEXCEPT;
float tanf(float) MATH_LIBM_NOEXCEPT;
float asinf(float) MATH_LIBM_NOEXCEPT;
float acosf(float) MATH_LIBM_NOEXCEPT;
float atan2f(float, float) MATH_LIBM_NOEXCEPT;
float atanf(float) MATH_LIBM_NOEXCEPT;
double floor(double) MATH_LIBM_NOEXCEPT;
double ceil(double) MATH_LIBM_NOEXCEPT;
double round(double) MATH_LIBM_NOEXCEPT;
double sqrt(double) MATH_LIBM_NOEXCEPT;
double pow(double, double) MATH_LIBM_NOEXCEPT;
double exp2(double) MATH_LIBM_NOEXCEPT;
double ldexp(double, int) MATH_LIBM_NOEXCEPT;
double log(double) MATH_LIBM_NOEXCEPT;
double sin(double) MATH_LIBM_NOEXCEPT;
double cos(double) MATH_LIBM_NOEXCEPT;
double tan(double) MATH_LIBM_NOEXCEPT;
double atan2(double, double) MATH_LIBM_NOEXCEPT;
double fabs(double) MATH_LIBM_NOEXCEPT;
double copysign(double, double) MATH_LIBM_NOEXCEPT;
}
#undef MATH_LIBM_NOEXCEPT

namespace math {

inline constexpr float radians(float deg) { return deg * (pi / 180.0f); }
inline constexpr double radians(double deg) { return deg * (piD / 180.0); }
inline constexpr float degrees(float rad) { return rad * (180.0f / pi); }
inline constexpr double degrees(double rad) { return rad * (180.0 / piD); }

inline constexpr float min(float a, float b) { return a < b ? a : b; }
inline constexpr double min(double a, double b) { return a < b ? a : b; }
inline constexpr int min(int a, int b) { return a < b ? a : b; }
inline constexpr uint8_t min(uint8_t a, uint8_t b) { return a < b ? a : b; }
inline constexpr uint16_t min(uint16_t a, uint16_t b) { return a < b ? a : b; }
inline constexpr int64_t min(int64_t a, int64_t b) { return a < b ? a : b; }
inline constexpr uint32_t min(uint32_t a, uint32_t b) { return a < b ? a : b; }
inline constexpr uint64_t min(uint64_t a, uint64_t b) { return a < b ? a : b; }
inline constexpr float max(float a, float b) { return a > b ? a : b; }
inline constexpr double max(double a, double b) { return a > b ? a : b; }
inline constexpr int max(int a, int b) { return a > b ? a : b; }
inline constexpr uint8_t max(uint8_t a, uint8_t b) { return a > b ? a : b; }
inline constexpr uint16_t max(uint16_t a, uint16_t b) { return a > b ? a : b; }
inline constexpr int64_t max(int64_t a, int64_t b) { return a > b ? a : b; }
inline constexpr uint32_t max(uint32_t a, uint32_t b) { return a > b ? a : b; }
inline constexpr uint64_t max(uint64_t a, uint64_t b) { return a > b ? a : b; }

inline constexpr float clamp(float v, float lo, float hi) { return min(max(v, lo), hi); }
inline constexpr double clamp(double v, double lo, double hi) { return min(max(v, lo), hi); }
inline constexpr int clamp(int v, int lo, int hi) { return min(max(v, lo), hi); }
inline constexpr uint32_t clamp(uint32_t v, uint32_t lo, uint32_t hi) { return min(max(v, lo), hi); }
inline constexpr uint8_t clamp(uint8_t v, uint8_t lo, uint8_t hi) { return (uint8_t)min(max((int)v, (int)lo), (int)hi); }
inline constexpr uint16_t clamp(uint16_t v, uint16_t lo, uint16_t hi) { return (uint16_t)min(max((int)v, (int)lo), (int)hi); }

inline constexpr float smoothstep(float edge0, float edge1, float x) {
	const float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}
inline constexpr double smoothstep(double edge0, double edge1, double x) {
	const double t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

inline constexpr float abs(float v) { return v < 0.0f ? -v : v; }
inline constexpr double abs(double v) { return v < 0.0 ? -v : v; }
inline constexpr int abs(int v) { return v < 0 ? -v : v; }

inline float floor(float v) { return floorf(v); }
inline double floor(double v) { return ::floor(v); }
inline constexpr int floor(int v) { return v; }
inline float ceil(float v) { return ceilf(v); }
inline double ceil(double v) { return ::ceil(v); }
inline constexpr int ceil(int v) { return v; }

// A C cast truncates toward zero, so a negative fraction needs one extra step.
inline constexpr int floorToInt(float value) {
	int whole = (int)value;
	if ((float)whole > value) {
		--whole;
	}
	return whole;
}

// A C cast truncates toward zero, so a positive fraction needs one extra step.
inline constexpr int ceilToInt(float value) {
	int whole = (int)value;
	if ((float)whole < value) {
		++whole;
	}
	return whole;
}
inline float round(float v) { return roundf(v); }
inline double round(double v) { return ::round(v); }
inline constexpr int round(int v) { return v; }
inline float fract(float v) { return v - floorf(v); }
inline float sqrt(float v) { return sqrtf(v); }
inline float inversesqrt(float v) { return 1.0f / sqrtf(v); }
inline double sqrt(double v) { return ::sqrt(v); }
inline float pow(float b, float e) { return powf(b, e); }
inline float pow(float b, int e) { return powf(b, (float)e); }
inline double pow(double b, double e) { return ::pow(b, e); }
inline double pow(double b, int e) { return ::pow(b, (double)e); }
inline constexpr int pow(int b, int e) {
	int r = 1;
	for (int i = 0; i < e; ++i) {
		r *= b;
	}
	return r;
}
inline float exp(float v) { return expf(v); }
inline float exp2(float v) { return exp2f(v); }
inline double exp2(double v) { return ::exp2(v); }
inline float ldexp(float v, int e) { return ldexpf(v, e); }
inline double ldexp(double v, int e) { return ::ldexp(v, e); }
inline float log(float v) { return logf(v); }
inline double log(double v) { return ::log(v); }
inline float log2(float v) { return log2f(v); }
inline float sin(float v) { return sinf(v); }
inline double sin(double v) { return ::sin(v); }
inline float cos(float v) { return cosf(v); }
inline double cos(double v) { return ::cos(v); }
inline float tan(float v) { return tanf(v); }
inline double tan(double v) { return ::tan(v); }
inline float asin(float v) { return asinf(v); }
inline float acos(float v) { return acosf(v); }
inline float atan(float y, float x) { return atan2f(y, x); }
inline float atan(float v) { return atanf(v); }
inline double atan(double y, double x) { return ::atan2(y, x); }

inline constexpr float mix(float a, float b, float t) { return a * (1.0f - t) + b * t; }
inline constexpr double mix(double a, double b, double t) { return a * (1.0 - t) + b * t; }
inline float mod(float a, float b) { return a - b * floorf(a / b); }
inline double mod(double a, double b) { return a - b * ::floor(a / b); }
inline constexpr int mod(int a, int b) { return a % b; }
inline constexpr float sign(float v) { return v < 0.0f ? -1.0f : (v > 0.0f ? 1.0f : 0.0f); }
inline constexpr double sign(double v) { return v < 0.0 ? -1.0 : (v > 0.0 ? 1.0 : 0.0); }

// v != v detects NaN only when fast-math is disabled. -ffast-math assumes no NaNs.
inline constexpr bool isnan(float v) { return v != v; }
inline constexpr bool isnan(double v) { return v != v; }
inline constexpr bool isinf(float v) { return v == v && v * 0.0f != 0.0f; }
inline constexpr bool isinf(double v) { return v == v && v * 0.0 != 0.0; }
inline constexpr bool isfinite(float v) { return !isnan(v) && !isinf(v); }
inline constexpr bool isfinite(double v) { return !isnan(v) && !isinf(v); }
inline constexpr bool epsilonEqual(float a, float b, float eps) { return abs(a - b) <= eps; }
inline constexpr bool epsilonEqual(double a, double b, double eps) { return abs(a - b) <= eps; }
inline constexpr bool epsilonNotEqual(float a, float b, float eps) { return abs(a - b) > eps; }
inline constexpr bool epsilonNotEqual(double a, double b, double eps) { return abs(a - b) > eps; }
inline constexpr bool isPowerOfTwo(int v) { return v > 0 && (v & (v - 1)) == 0; }
inline constexpr bool isPowerOfTwo(uint32_t v) { return v > 0u && (v & (v - 1u)) == 0u; }
inline constexpr bool isPowerOfTwo(uint64_t v) { return v > 0ull && (v & (v - 1ull)) == 0ull; }

inline float uintBitsToFloat(uint32_t v) {
	union {
		uint32_t in;
		float out;
	} u;
	u.in = v;
	return u.out;
}

}
