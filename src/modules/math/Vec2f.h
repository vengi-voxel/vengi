/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec2.h"
#include "math/Vec2i.h"

namespace math {
struct Vec3f;
struct Vec4f;

struct Vec2f {
	union {
		struct { float x, y; };
		struct { float r, g; };
	};
	typedef Vec2f col_type;
	typedef float value_type;
	static constexpr int length() { return 2; }

	Vec2f() = default;
	constexpr Vec2f(float s) : x(s), y(s) {}
	constexpr Vec2f(float x_, float y_) : x(x_), y(y_) {}

	constexpr float &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		return x;
	}
	constexpr float operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		return x;
	}
	constexpr Vec2f(const Vec2i &v);
	constexpr Vec2f(const Vec3f &v);
	constexpr Vec2f(const Vec4f &v);
	constexpr Vec2f &operator+=(const Vec2f &o) { x += o.x; y += o.y; return *this; }
	constexpr Vec2f &operator+=(float s) { x += s; y += s; return *this; }
	constexpr Vec2f &operator-=(const Vec2f &o) { x -= o.x; y -= o.y; return *this; }
	constexpr Vec2f &operator-=(float s) { x -= s; y -= s; return *this; }
	constexpr Vec2f &operator*=(const Vec2f &o) { x *= o.x; y *= o.y; return *this; }
	constexpr Vec2f &operator*=(float s) { x *= s; y *= s; return *this; }
	constexpr Vec2f &operator/=(const Vec2f &o) { x /= o.x; y /= o.y; return *this; }
	constexpr Vec2f &operator/=(float s) { x /= s; y /= s; return *this; }
};

inline constexpr Vec2f operator+(const Vec2f &a, const Vec2f &o) { return Vec2f(a.x + o.x, a.y + o.y); }
inline constexpr Vec2f operator+(const Vec2f &a, float s) { return Vec2f(a.x + s, a.y + s); }
inline constexpr Vec2f operator+(float s, const Vec2f &a) { return Vec2f(s + a.x, s + a.y); }
inline constexpr Vec2f operator-(const Vec2f &a, const Vec2f &o) { return Vec2f(a.x - o.x, a.y - o.y); }
inline constexpr Vec2f operator-(const Vec2f &a, float s) { return Vec2f(a.x - s, a.y - s); }
inline constexpr Vec2f operator-(float s, const Vec2f &a) { return Vec2f(s - a.x, s - a.y); }
inline constexpr Vec2f operator*(const Vec2f &a, const Vec2f &o) { return Vec2f(a.x * o.x, a.y * o.y); }
inline constexpr Vec2f operator*(const Vec2f &a, float s) { return Vec2f(a.x * s, a.y * s); }
inline constexpr Vec2f operator*(float s, const Vec2f &a) { return Vec2f(s * a.x, s * a.y); }
inline constexpr Vec2f operator/(const Vec2f &a, const Vec2f &o) { return Vec2f(a.x / o.x, a.y / o.y); }
inline constexpr Vec2f operator/(const Vec2f &a, float s) { return Vec2f(a.x / s, a.y / s); }
inline constexpr Vec2f operator/(float s, const Vec2f &a) { return Vec2f(s / a.x, s / a.y); }
inline constexpr Vec2f operator-(const Vec2f &a) { return Vec2f(-a.x, -a.y); }
inline constexpr bool operator==(const Vec2f &a, const Vec2f &o) { return a.x == o.x && a.y == o.y; }
inline constexpr bool operator!=(const Vec2f &a, const Vec2f &o) { return !(a == o); }
inline constexpr float dot(const Vec2f &a, const Vec2f &b) { return a.x * b.x + a.y * b.y; }
inline constexpr Vec2f min(const Vec2f &a, const Vec2f &b) { return Vec2f(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y); }
inline constexpr Vec2f max(const Vec2f &a, const Vec2f &b) { return Vec2f(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y); }

inline constexpr BVec2 lessThan(const Vec2f &a, const Vec2f &b) { return BVec2(a.x < b.x, a.y < b.y); }
inline constexpr BVec2 lessThanEqual(const Vec2f &a, const Vec2f &b) { return BVec2(a.x <= b.x, a.y <= b.y); }
inline constexpr BVec2 greaterThan(const Vec2f &a, const Vec2f &b) { return BVec2(a.x > b.x, a.y > b.y); }
inline constexpr BVec2 greaterThanEqual(const Vec2f &a, const Vec2f &b) { return BVec2(a.x >= b.x, a.y >= b.y); }
inline constexpr BVec2 equal(const Vec2f &a, const Vec2f &b) { return BVec2(a.x == b.x, a.y == b.y); }

inline constexpr Vec2f abs(const Vec2f &a) { return Vec2f(abs(a.x), abs(a.y)); }
inline float length(const Vec2f &a) { return sqrt(dot(a, a)); }
inline constexpr float length2(const Vec2f &a) { return dot(a, a); }
inline float distance(const Vec2f &a, const Vec2f &b) { return length(a - b); }
inline constexpr float distance2(const Vec2f &a, const Vec2f &b) { return length2(a - b); }
inline Vec2f normalize(const Vec2f &a) { float len = length(a); return len > 0.0f ? a / len : Vec2f(0.0f); }
inline Vec2f floor(const Vec2f &a) { return Vec2f(floor(a.x), floor(a.y)); }
inline Vec2f ceil(const Vec2f &a) { return Vec2f(ceil(a.x), ceil(a.y)); }
inline Vec2f round(const Vec2f &a) { return Vec2f(round(a.x), round(a.y)); }
inline Vec2f fract(const Vec2f &a) { return Vec2f(fract(a.x), fract(a.y)); }
inline constexpr Vec2f mix(const Vec2f &a, const Vec2f &b, float t) { return Vec2f(mix(a.x, b.x, t), mix(a.y, b.y, t)); }
inline constexpr Vec2f clamp(const Vec2f &v, float lo, float hi) { return Vec2f(clamp(v.x, lo, hi), clamp(v.y, lo, hi)); }
inline constexpr Vec2f clamp(const Vec2f &v, const Vec2f &lo, const Vec2f &hi) { return Vec2f(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y)); }
inline constexpr Vec2f min(const Vec2f &a, float s) { return min(a, Vec2f(s)); }
inline constexpr Vec2f max(const Vec2f &a, float s) { return max(a, Vec2f(s)); }
inline constexpr BVec2 epsilonEqual(const Vec2f &a, const Vec2f &b, float eps) { return BVec2(epsilonEqual(a.x, b.x, eps), epsilonEqual(a.y, b.y, eps)); }
inline constexpr BVec2 epsilonEqual(const Vec2f &a, const Vec2f &b, const Vec2f &eps) { return BVec2(epsilonEqual(a.x, b.x, eps.x), epsilonEqual(a.y, b.y, eps.y)); }
inline constexpr BVec2 epsilonNotEqual(const Vec2f &a, const Vec2f &b, float eps) { return BVec2(epsilonNotEqual(a.x, b.x, eps), epsilonNotEqual(a.y, b.y, eps)); }
inline constexpr BVec2 isnan(const Vec2f &a) { return BVec2(isnan(a.x), isnan(a.y)); }
inline BVec2 isinf(const Vec2f &a) { return BVec2(isinf(a.x), isinf(a.y)); }
inline constexpr BVec2 isfinite(const Vec2f &a) { return BVec2(isfinite(a.x), isfinite(a.y)); }
inline Vec2f sin(const Vec2f &a) { return Vec2f((float)sin(a.x), (float)sin(a.y)); }
inline Vec2f cos(const Vec2f &a) { return Vec2f((float)cos(a.x), (float)cos(a.y)); }
inline Vec2f tan(const Vec2f &a) { return Vec2f((float)tan((float)a.x), (float)tan((float)a.y)); }
inline constexpr Vec2f radians(const Vec2f &a) { return Vec2f((float)radians(a.x), (float)radians(a.y)); }
inline constexpr Vec2f degrees(const Vec2f &a) { return Vec2f((float)degrees(a.x), (float)degrees(a.y)); }
inline Vec2f sqrt(const Vec2f &a) { return Vec2f((float)sqrt(a.x), (float)sqrt(a.y)); }
inline Vec2f pow(const Vec2f &a, float e) { return Vec2f((float)pow((float)a.x, (float)e), (float)pow((float)a.y, (float)e)); }
inline constexpr Vec2f sign(const Vec2f &a) { return Vec2f(sign((float)a.x), sign((float)a.y)); }
inline Vec2f mod(const Vec2f &a, float b) { return Vec2f(mod((float)a.x, (float)b), mod((float)a.y, (float)b)); }
inline Vec2f mod(const Vec2f &a, const Vec2f &b) { return Vec2f(mod(a.x, b.x), mod(a.y, b.y)); }
inline constexpr const float *value_ptr(const Vec2f &v) { return &v.x; }
inline constexpr float *value_ptr(Vec2f &v) { return &v.x; }


inline constexpr Vec2f::Vec2f(const Vec2i &v) : x((float)v.x), y((float)v.y) {}
inline constexpr Vec2i::Vec2i(const Vec2f &v) : x((int)v.x), y((int)v.y) {}
inline float angle(const Vec2f &a, const Vec2f &b) { return acos(clamp(dot(a, b), -1.0f, 1.0f)); }

}
