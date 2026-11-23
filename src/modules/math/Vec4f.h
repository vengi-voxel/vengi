/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec4.h"
#include "math/Vec3f.h"
#include "math/Vec4i.h"

namespace math {
struct AlignedVec4f;
struct AlignedVec4i;
struct Vec2f;

struct Vec4f {
	union {
		struct { float x, y, z, w; };
		struct { float r, g, b, a; };
	};
	typedef Vec4f col_type;
	typedef float value_type;
	static constexpr int length() { return 4; }

	Vec4f() = default;
	constexpr Vec4f(float s) : x(s), y(s), z(s), w(s) {}
	constexpr Vec4f(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

	constexpr float &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr float operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr Vec4f(const Vec2f &xy, float z_, float w_);
	constexpr Vec4f(float x_, float y_, const Vec2f &zw);
	constexpr Vec4f(const Vec2f &xy, const Vec2f &zw);
	constexpr Vec4f(const Vec3f &v, float w_);
	constexpr Vec4f(const Vec4i &v);
	constexpr Vec4f(const AlignedVec4f &v);
	constexpr Vec4f(const AlignedVec4i &v);
	constexpr Vec4f &operator+=(const Vec4f &o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr Vec4f &operator+=(float s) { x += s; y += s; z += s; w += s; return *this; }
	constexpr Vec4f &operator-=(const Vec4f &o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr Vec4f &operator-=(float s) { x -= s; y -= s; z -= s; w -= s; return *this; }
	constexpr Vec4f &operator*=(const Vec4f &o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr Vec4f &operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr Vec4f &operator/=(const Vec4f &o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
	constexpr Vec4f &operator/=(float s) { x /= s; y /= s; z /= s; w /= s; return *this; }
};

inline constexpr Vec4f operator+(const Vec4f &a, const Vec4f &o) { return Vec4f(a.x + o.x, a.y + o.y, a.z + o.z, a.w + o.w); }
inline constexpr Vec4f operator+(const Vec4f &a, float s) { return Vec4f(a.x + s, a.y + s, a.z + s, a.w + s); }
inline constexpr Vec4f operator+(float s, const Vec4f &a) { return Vec4f(s + a.x, s + a.y, s + a.z, s + a.w); }
inline constexpr Vec4f operator-(const Vec4f &a, const Vec4f &o) { return Vec4f(a.x - o.x, a.y - o.y, a.z - o.z, a.w - o.w); }
inline constexpr Vec4f operator-(const Vec4f &a, float s) { return Vec4f(a.x - s, a.y - s, a.z - s, a.w - s); }
inline constexpr Vec4f operator-(float s, const Vec4f &a) { return Vec4f(s - a.x, s - a.y, s - a.z, s - a.w); }
inline constexpr Vec4f operator*(const Vec4f &a, const Vec4f &o) { return Vec4f(a.x * o.x, a.y * o.y, a.z * o.z, a.w * o.w); }
inline constexpr Vec4f operator*(const Vec4f &a, float s) { return Vec4f(a.x * s, a.y * s, a.z * s, a.w * s); }
inline constexpr Vec4f operator*(float s, const Vec4f &a) { return Vec4f(s * a.x, s * a.y, s * a.z, s * a.w); }
inline constexpr Vec4f operator/(const Vec4f &a, const Vec4f &o) { return Vec4f(a.x / o.x, a.y / o.y, a.z / o.z, a.w / o.w); }
inline constexpr Vec4f operator/(const Vec4f &a, float s) { return Vec4f(a.x / s, a.y / s, a.z / s, a.w / s); }
inline constexpr Vec4f operator/(float s, const Vec4f &a) { return Vec4f(s / a.x, s / a.y, s / a.z, s / a.w); }
inline constexpr Vec4f operator-(const Vec4f &a) { return Vec4f(-a.x, -a.y, -a.z, -a.w); }
inline constexpr bool operator==(const Vec4f &a, const Vec4f &o) { return a.x == o.x && a.y == o.y && a.z == o.z && a.w == o.w; }
inline constexpr bool operator!=(const Vec4f &a, const Vec4f &o) { return !(a == o); }
inline constexpr float dot(const Vec4f &a, const Vec4f &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline constexpr Vec4f min(const Vec4f &a, const Vec4f &b) { return Vec4f(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w); }
inline constexpr Vec4f max(const Vec4f &a, const Vec4f &b) { return Vec4f(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w); }

inline constexpr BVec4 lessThan(const Vec4f &a, const Vec4f &b) { return BVec4(a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w); }
inline constexpr BVec4 lessThanEqual(const Vec4f &a, const Vec4f &b) { return BVec4(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); }
inline constexpr BVec4 greaterThan(const Vec4f &a, const Vec4f &b) { return BVec4(a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w); }
inline constexpr BVec4 greaterThanEqual(const Vec4f &a, const Vec4f &b) { return BVec4(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }
inline constexpr BVec4 equal(const Vec4f &a, const Vec4f &b) { return BVec4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); }

inline constexpr Vec4f abs(const Vec4f &a) { return Vec4f(abs(a.x), abs(a.y), abs(a.z), abs(a.w)); }
inline float length(const Vec4f &a) { return sqrt(dot(a, a)); }
inline constexpr float length2(const Vec4f &a) { return dot(a, a); }
inline float distance(const Vec4f &a, const Vec4f &b) { return length(a - b); }
inline constexpr float distance2(const Vec4f &a, const Vec4f &b) { return length2(a - b); }
inline Vec4f normalize(const Vec4f &a) { float len = length(a); return len > 0.0f ? a / len : Vec4f(0.0f); }
inline Vec4f floor(const Vec4f &a) { return Vec4f(floor(a.x), floor(a.y), floor(a.z), floor(a.w)); }
inline Vec4f ceil(const Vec4f &a) { return Vec4f(ceil(a.x), ceil(a.y), ceil(a.z), ceil(a.w)); }
inline Vec4f round(const Vec4f &a) { return Vec4f(round(a.x), round(a.y), round(a.z), round(a.w)); }
inline Vec4f fract(const Vec4f &a) { return Vec4f(fract(a.x), fract(a.y), fract(a.z), fract(a.w)); }
inline constexpr Vec4f mix(const Vec4f &a, const Vec4f &b, float t) { return Vec4f(mix(a.x, b.x, t), mix(a.y, b.y, t), mix(a.z, b.z, t), mix(a.w, b.w, t)); }
inline constexpr Vec4f clamp(const Vec4f &v, float lo, float hi) { return Vec4f(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi)); }
inline constexpr Vec4f clamp(const Vec4f &v, const Vec4f &lo, const Vec4f &hi) { return Vec4f(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w)); }
inline constexpr Vec4f min(const Vec4f &a, float s) { return min(a, Vec4f(s)); }
inline constexpr Vec4f max(const Vec4f &a, float s) { return max(a, Vec4f(s)); }
inline constexpr BVec4 epsilonEqual(const Vec4f &a, const Vec4f &b, float eps) { return BVec4(epsilonEqual(a.x, b.x, eps), epsilonEqual(a.y, b.y, eps), epsilonEqual(a.z, b.z, eps), epsilonEqual(a.w, b.w, eps)); }
inline constexpr BVec4 epsilonEqual(const Vec4f &a, const Vec4f &b, const Vec4f &eps) { return BVec4(epsilonEqual(a.x, b.x, eps.x), epsilonEqual(a.y, b.y, eps.y), epsilonEqual(a.z, b.z, eps.z), epsilonEqual(a.w, b.w, eps.w)); }
inline constexpr BVec4 epsilonNotEqual(const Vec4f &a, const Vec4f &b, float eps) { return BVec4(epsilonNotEqual(a.x, b.x, eps), epsilonNotEqual(a.y, b.y, eps), epsilonNotEqual(a.z, b.z, eps), epsilonNotEqual(a.w, b.w, eps)); }
inline constexpr BVec4 isnan(const Vec4f &a) { return BVec4(isnan(a.x), isnan(a.y), isnan(a.z), isnan(a.w)); }
inline BVec4 isinf(const Vec4f &a) { return BVec4(isinf(a.x), isinf(a.y), isinf(a.z), isinf(a.w)); }
inline constexpr BVec4 isfinite(const Vec4f &a) { return BVec4(isfinite(a.x), isfinite(a.y), isfinite(a.z), isfinite(a.w)); }
inline Vec4f sin(const Vec4f &a) { return Vec4f((float)sin(a.x), (float)sin(a.y), (float)sin(a.z), (float)sin(a.w)); }
inline Vec4f cos(const Vec4f &a) { return Vec4f((float)cos(a.x), (float)cos(a.y), (float)cos(a.z), (float)cos(a.w)); }
inline Vec4f tan(const Vec4f &a) { return Vec4f((float)tan((float)a.x), (float)tan((float)a.y), (float)tan((float)a.z), (float)tan((float)a.w)); }
inline constexpr Vec4f radians(const Vec4f &a) { return Vec4f((float)radians(a.x), (float)radians(a.y), (float)radians(a.z), (float)radians(a.w)); }
inline constexpr Vec4f degrees(const Vec4f &a) { return Vec4f((float)degrees(a.x), (float)degrees(a.y), (float)degrees(a.z), (float)degrees(a.w)); }
inline Vec4f sqrt(const Vec4f &a) { return Vec4f((float)sqrt(a.x), (float)sqrt(a.y), (float)sqrt(a.z), (float)sqrt(a.w)); }
inline Vec4f pow(const Vec4f &a, float e) { return Vec4f((float)pow((float)a.x, (float)e), (float)pow((float)a.y, (float)e), (float)pow((float)a.z, (float)e), (float)pow((float)a.w, (float)e)); }
inline constexpr Vec4f sign(const Vec4f &a) { return Vec4f(sign((float)a.x), sign((float)a.y), sign((float)a.z), sign((float)a.w)); }
inline Vec4f mod(const Vec4f &a, float b) { return Vec4f(mod((float)a.x, (float)b), mod((float)a.y, (float)b), mod((float)a.z, (float)b), mod((float)a.w, (float)b)); }
inline Vec4f mod(const Vec4f &a, const Vec4f &b) { return Vec4f(mod(a.x, b.x), mod(a.y, b.y), mod(a.z, b.z), mod(a.w, b.w)); }
inline constexpr const float *value_ptr(const Vec4f &v) { return &v.x; }
inline constexpr float *value_ptr(Vec4f &v) { return &v.x; }


inline constexpr Vec4f::Vec4f(const Vec3f &v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
inline constexpr Vec4f::Vec4f(const Vec4i &v) : x((float)v.x), y((float)v.y), z((float)v.z), w((float)v.w) {}
inline constexpr Vec3f::Vec3f(const Vec4f &v) : x(v.x), y(v.y), z(v.z) {}
inline constexpr Vec4i::Vec4i(const Vec4f &v) : x((int)v.x), y((int)v.y), z((int)v.z), w((int)v.w) {}

}
