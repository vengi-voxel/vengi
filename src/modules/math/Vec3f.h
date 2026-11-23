/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec3.h"
#include "math/Vec3i.h"

namespace math {
struct Vec3u;
struct Vec3d;
struct Vec4f;
struct Vec2f;
struct AlignedVec4f;

struct Vec3f {
	union {
		struct { float x, y, z; };
		struct { float r, g, b; };
	};
	typedef Vec3f col_type;
	typedef float value_type;
	static constexpr int length() { return 3; }

	Vec3f() = default;
	constexpr Vec3f(float s) : x(s), y(s), z(s) {}
	constexpr Vec3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

	constexpr float &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr float operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr Vec3f(const Vec3i &v);
	constexpr Vec3f(const Vec3u &v);
	constexpr Vec3f(const Vec3d &v);
	constexpr Vec3f(const Vec4f &v);
	constexpr Vec3f(const Vec2f &v, float z_);
	constexpr Vec3f(const AlignedVec4f &v);
	constexpr Vec3f &operator+=(const Vec3f &o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr Vec3f &operator+=(float s) { x += s; y += s; z += s; return *this; }
	constexpr Vec3f &operator-=(const Vec3f &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr Vec3f &operator-=(float s) { x -= s; y -= s; z -= s; return *this; }
	constexpr Vec3f &operator*=(const Vec3f &o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr Vec3f &operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
	constexpr Vec3f &operator/=(const Vec3f &o) { x /= o.x; y /= o.y; z /= o.z; return *this; }
	constexpr Vec3f &operator/=(float s) { x /= s; y /= s; z /= s; return *this; }
};

inline constexpr Vec3f operator+(const Vec3f &a, const Vec3f &o) { return Vec3f(a.x + o.x, a.y + o.y, a.z + o.z); }
inline constexpr Vec3f operator+(const Vec3f &a, float s) { return Vec3f(a.x + s, a.y + s, a.z + s); }
inline constexpr Vec3f operator+(float s, const Vec3f &a) { return Vec3f(s + a.x, s + a.y, s + a.z); }
inline constexpr Vec3f operator-(const Vec3f &a, const Vec3f &o) { return Vec3f(a.x - o.x, a.y - o.y, a.z - o.z); }
inline constexpr Vec3f operator-(const Vec3f &a, float s) { return Vec3f(a.x - s, a.y - s, a.z - s); }
inline constexpr Vec3f operator-(float s, const Vec3f &a) { return Vec3f(s - a.x, s - a.y, s - a.z); }
inline constexpr Vec3f operator*(const Vec3f &a, const Vec3f &o) { return Vec3f(a.x * o.x, a.y * o.y, a.z * o.z); }
inline constexpr Vec3f operator*(const Vec3f &a, float s) { return Vec3f(a.x * s, a.y * s, a.z * s); }
inline constexpr Vec3f operator*(float s, const Vec3f &a) { return Vec3f(s * a.x, s * a.y, s * a.z); }
inline constexpr Vec3f operator/(const Vec3f &a, const Vec3f &o) { return Vec3f(a.x / o.x, a.y / o.y, a.z / o.z); }
inline constexpr Vec3f operator/(const Vec3f &a, float s) { return Vec3f(a.x / s, a.y / s, a.z / s); }
inline constexpr Vec3f operator/(float s, const Vec3f &a) { return Vec3f(s / a.x, s / a.y, s / a.z); }
inline constexpr Vec3f operator-(const Vec3f &a) { return Vec3f(-a.x, -a.y, -a.z); }
inline constexpr bool operator==(const Vec3f &a, const Vec3f &o) { return a.x == o.x && a.y == o.y && a.z == o.z; }
inline constexpr bool operator!=(const Vec3f &a, const Vec3f &o) { return !(a == o); }
inline constexpr float dot(const Vec3f &a, const Vec3f &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr Vec3f min(const Vec3f &a, const Vec3f &b) { return Vec3f(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z); }
inline constexpr Vec3f max(const Vec3f &a, const Vec3f &b) { return Vec3f(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z); }

inline constexpr BVec3 lessThan(const Vec3f &a, const Vec3f &b) { return BVec3(a.x < b.x, a.y < b.y, a.z < b.z); }
inline constexpr BVec3 lessThanEqual(const Vec3f &a, const Vec3f &b) { return BVec3(a.x <= b.x, a.y <= b.y, a.z <= b.z); }
inline constexpr BVec3 greaterThan(const Vec3f &a, const Vec3f &b) { return BVec3(a.x > b.x, a.y > b.y, a.z > b.z); }
inline constexpr BVec3 greaterThanEqual(const Vec3f &a, const Vec3f &b) { return BVec3(a.x >= b.x, a.y >= b.y, a.z >= b.z); }
inline constexpr BVec3 equal(const Vec3f &a, const Vec3f &b) { return BVec3(a.x == b.x, a.y == b.y, a.z == b.z); }
inline constexpr Vec3f cross(const Vec3f &a, const Vec3f &b) { return Vec3f(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
inline constexpr Vec3f abs(const Vec3f &a) { return Vec3f(abs(a.x), abs(a.y), abs(a.z)); }
inline float length(const Vec3f &a) { return sqrt(dot(a, a)); }
inline constexpr float length2(const Vec3f &a) { return dot(a, a); }
inline float distance(const Vec3f &a, const Vec3f &b) { return length(a - b); }
inline constexpr float distance2(const Vec3f &a, const Vec3f &b) { return length2(a - b); }
inline Vec3f normalize(const Vec3f &a) { float len = length(a); return len > 0.0f ? a / len : Vec3f(0.0f); }
inline Vec3f floor(const Vec3f &a) { return Vec3f(floor(a.x), floor(a.y), floor(a.z)); }
inline Vec3f ceil(const Vec3f &a) { return Vec3f(ceil(a.x), ceil(a.y), ceil(a.z)); }
inline Vec3f round(const Vec3f &a) { return Vec3f(round(a.x), round(a.y), round(a.z)); }
inline Vec3f fract(const Vec3f &a) { return Vec3f(fract(a.x), fract(a.y), fract(a.z)); }
inline constexpr Vec3f mix(const Vec3f &a, const Vec3f &b, float t) { return Vec3f(mix(a.x, b.x, t), mix(a.y, b.y, t), mix(a.z, b.z, t)); }
inline constexpr Vec3f clamp(const Vec3f &v, float lo, float hi) { return Vec3f(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi)); }
inline constexpr Vec3f clamp(const Vec3f &v, const Vec3f &lo, const Vec3f &hi) { return Vec3f(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z)); }
inline constexpr Vec3f min(const Vec3f &a, float s) { return min(a, Vec3f(s)); }
inline constexpr Vec3f max(const Vec3f &a, float s) { return max(a, Vec3f(s)); }
inline constexpr BVec3 epsilonEqual(const Vec3f &a, const Vec3f &b, float eps) { return BVec3(epsilonEqual(a.x, b.x, eps), epsilonEqual(a.y, b.y, eps), epsilonEqual(a.z, b.z, eps)); }
inline constexpr BVec3 epsilonEqual(const Vec3f &a, const Vec3f &b, const Vec3f &eps) { return BVec3(epsilonEqual(a.x, b.x, eps.x), epsilonEqual(a.y, b.y, eps.y), epsilonEqual(a.z, b.z, eps.z)); }
inline constexpr BVec3 epsilonNotEqual(const Vec3f &a, const Vec3f &b, float eps) { return BVec3(epsilonNotEqual(a.x, b.x, eps), epsilonNotEqual(a.y, b.y, eps), epsilonNotEqual(a.z, b.z, eps)); }
inline constexpr BVec3 isnan(const Vec3f &a) { return BVec3(isnan(a.x), isnan(a.y), isnan(a.z)); }
inline BVec3 isinf(const Vec3f &a) { return BVec3(isinf(a.x), isinf(a.y), isinf(a.z)); }
inline constexpr BVec3 isfinite(const Vec3f &a) { return BVec3(isfinite(a.x), isfinite(a.y), isfinite(a.z)); }
inline Vec3f sin(const Vec3f &a) { return Vec3f((float)sin(a.x), (float)sin(a.y), (float)sin(a.z)); }
inline Vec3f cos(const Vec3f &a) { return Vec3f((float)cos(a.x), (float)cos(a.y), (float)cos(a.z)); }
inline Vec3f tan(const Vec3f &a) { return Vec3f((float)tan((float)a.x), (float)tan((float)a.y), (float)tan((float)a.z)); }
inline constexpr Vec3f radians(const Vec3f &a) { return Vec3f((float)radians(a.x), (float)radians(a.y), (float)radians(a.z)); }
inline constexpr Vec3f degrees(const Vec3f &a) { return Vec3f((float)degrees(a.x), (float)degrees(a.y), (float)degrees(a.z)); }
inline Vec3f sqrt(const Vec3f &a) { return Vec3f((float)sqrt(a.x), (float)sqrt(a.y), (float)sqrt(a.z)); }
inline Vec3f pow(const Vec3f &a, float e) { return Vec3f((float)pow((float)a.x, (float)e), (float)pow((float)a.y, (float)e), (float)pow((float)a.z, (float)e)); }
inline constexpr Vec3f sign(const Vec3f &a) { return Vec3f(sign((float)a.x), sign((float)a.y), sign((float)a.z)); }
inline Vec3f mod(const Vec3f &a, float b) { return Vec3f(mod((float)a.x, (float)b), mod((float)a.y, (float)b), mod((float)a.z, (float)b)); }
inline Vec3f mod(const Vec3f &a, const Vec3f &b) { return Vec3f(mod(a.x, b.x), mod(a.y, b.y), mod(a.z, b.z)); }
inline constexpr const float *value_ptr(const Vec3f &v) { return &v.x; }
inline constexpr float *value_ptr(Vec3f &v) { return &v.x; }


inline constexpr Vec3f::Vec3f(const Vec3i &v) : x((float)v.x), y((float)v.y), z((float)v.z) {}
inline constexpr Vec3i::Vec3i(const Vec3f &v) : x((int)v.x), y((int)v.y), z((int)v.z) {}
inline float angle(const Vec3f &a, const Vec3f &b) { return acos(clamp(dot(a, b), -1.0f, 1.0f)); }

inline Vec3f rotateX(const Vec3f &v, float angle) {
	const float c = cos(angle);
	const float s = sin(angle);
	return Vec3f(v.x, v.y * c - v.z * s, v.y * s + v.z * c);
}
inline Vec3f rotateY(const Vec3f &v, float angle) {
	const float c = cos(angle);
	const float s = sin(angle);
	return Vec3f(v.x * c + v.z * s, v.y, -v.x * s + v.z * c);
}
inline Vec3f rotateZ(const Vec3f &v, float angle) {
	const float c = cos(angle);
	const float s = sin(angle);
	return Vec3f(v.x * c - v.y * s, v.x * s + v.y * c, v.z);
}

}
