/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec4.h"

namespace math {
struct Vec4f;
struct Vec3f;
struct AlignedVec4i;

struct alignas(16) AlignedVec4f {
	union {
		struct { float x, y, z, w; };
		struct { float r, g, b, a; };
	};
	typedef AlignedVec4f col_type;
	typedef float value_type;
	static constexpr int length() { return 4; }

	AlignedVec4f() = default;
	constexpr AlignedVec4f(float s) : x(s), y(s), z(s), w(s) {}
	constexpr AlignedVec4f(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

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
	constexpr AlignedVec4f(const Vec4f &v);
	constexpr AlignedVec4f(const Vec3f &v, float w_);
	constexpr AlignedVec4f(const AlignedVec4i &v);
	constexpr AlignedVec4f &operator+=(const AlignedVec4f &o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr AlignedVec4f &operator+=(float s) { x += s; y += s; z += s; w += s; return *this; }
	constexpr AlignedVec4f &operator-=(const AlignedVec4f &o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr AlignedVec4f &operator-=(float s) { x -= s; y -= s; z -= s; w -= s; return *this; }
	constexpr AlignedVec4f &operator*=(const AlignedVec4f &o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr AlignedVec4f &operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr AlignedVec4f &operator/=(const AlignedVec4f &o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
	constexpr AlignedVec4f &operator/=(float s) { x /= s; y /= s; z /= s; w /= s; return *this; }
};

inline constexpr AlignedVec4f operator+(const AlignedVec4f &a, const AlignedVec4f &o) { return AlignedVec4f(a.x + o.x, a.y + o.y, a.z + o.z, a.w + o.w); }
inline constexpr AlignedVec4f operator+(const AlignedVec4f &a, float s) { return AlignedVec4f(a.x + s, a.y + s, a.z + s, a.w + s); }
inline constexpr AlignedVec4f operator+(float s, const AlignedVec4f &a) { return AlignedVec4f(s + a.x, s + a.y, s + a.z, s + a.w); }
inline constexpr AlignedVec4f operator-(const AlignedVec4f &a, const AlignedVec4f &o) { return AlignedVec4f(a.x - o.x, a.y - o.y, a.z - o.z, a.w - o.w); }
inline constexpr AlignedVec4f operator-(const AlignedVec4f &a, float s) { return AlignedVec4f(a.x - s, a.y - s, a.z - s, a.w - s); }
inline constexpr AlignedVec4f operator-(float s, const AlignedVec4f &a) { return AlignedVec4f(s - a.x, s - a.y, s - a.z, s - a.w); }
inline constexpr AlignedVec4f operator*(const AlignedVec4f &a, const AlignedVec4f &o) { return AlignedVec4f(a.x * o.x, a.y * o.y, a.z * o.z, a.w * o.w); }
inline constexpr AlignedVec4f operator*(const AlignedVec4f &a, float s) { return AlignedVec4f(a.x * s, a.y * s, a.z * s, a.w * s); }
inline constexpr AlignedVec4f operator*(float s, const AlignedVec4f &a) { return AlignedVec4f(s * a.x, s * a.y, s * a.z, s * a.w); }
inline constexpr AlignedVec4f operator/(const AlignedVec4f &a, const AlignedVec4f &o) { return AlignedVec4f(a.x / o.x, a.y / o.y, a.z / o.z, a.w / o.w); }
inline constexpr AlignedVec4f operator/(const AlignedVec4f &a, float s) { return AlignedVec4f(a.x / s, a.y / s, a.z / s, a.w / s); }
inline constexpr AlignedVec4f operator/(float s, const AlignedVec4f &a) { return AlignedVec4f(s / a.x, s / a.y, s / a.z, s / a.w); }
inline constexpr AlignedVec4f operator-(const AlignedVec4f &a) { return AlignedVec4f(-a.x, -a.y, -a.z, -a.w); }
inline constexpr bool operator==(const AlignedVec4f &a, const AlignedVec4f &o) { return a.x == o.x && a.y == o.y && a.z == o.z && a.w == o.w; }
inline constexpr bool operator!=(const AlignedVec4f &a, const AlignedVec4f &o) { return !(a == o); }
inline constexpr float dot(const AlignedVec4f &a, const AlignedVec4f &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline constexpr AlignedVec4f min(const AlignedVec4f &a, const AlignedVec4f &b) { return AlignedVec4f(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w); }
inline constexpr AlignedVec4f max(const AlignedVec4f &a, const AlignedVec4f &b) { return AlignedVec4f(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w); }

inline constexpr BVec4 lessThan(const AlignedVec4f &a, const AlignedVec4f &b) { return BVec4(a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w); }
inline constexpr BVec4 lessThanEqual(const AlignedVec4f &a, const AlignedVec4f &b) { return BVec4(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); }
inline constexpr BVec4 greaterThan(const AlignedVec4f &a, const AlignedVec4f &b) { return BVec4(a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w); }
inline constexpr BVec4 greaterThanEqual(const AlignedVec4f &a, const AlignedVec4f &b) { return BVec4(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }
inline constexpr BVec4 equal(const AlignedVec4f &a, const AlignedVec4f &b) { return BVec4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); }

inline constexpr AlignedVec4f abs(const AlignedVec4f &a) { return AlignedVec4f(abs(a.x), abs(a.y), abs(a.z), abs(a.w)); }
inline float length(const AlignedVec4f &a) { return sqrt(dot(a, a)); }
inline constexpr float length2(const AlignedVec4f &a) { return dot(a, a); }
inline float distance(const AlignedVec4f &a, const AlignedVec4f &b) { return length(a - b); }
inline constexpr float distance2(const AlignedVec4f &a, const AlignedVec4f &b) { return length2(a - b); }
inline AlignedVec4f normalize(const AlignedVec4f &a) { float len = length(a); return len > 0.0f ? a / len : AlignedVec4f(0.0f); }
inline AlignedVec4f floor(const AlignedVec4f &a) { return AlignedVec4f(floor(a.x), floor(a.y), floor(a.z), floor(a.w)); }
inline AlignedVec4f ceil(const AlignedVec4f &a) { return AlignedVec4f(ceil(a.x), ceil(a.y), ceil(a.z), ceil(a.w)); }
inline AlignedVec4f round(const AlignedVec4f &a) { return AlignedVec4f(round(a.x), round(a.y), round(a.z), round(a.w)); }
inline AlignedVec4f fract(const AlignedVec4f &a) { return AlignedVec4f(fract(a.x), fract(a.y), fract(a.z), fract(a.w)); }
inline constexpr AlignedVec4f mix(const AlignedVec4f &a, const AlignedVec4f &b, float t) { return AlignedVec4f(mix(a.x, b.x, t), mix(a.y, b.y, t), mix(a.z, b.z, t), mix(a.w, b.w, t)); }
inline constexpr AlignedVec4f clamp(const AlignedVec4f &v, float lo, float hi) { return AlignedVec4f(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi)); }
inline constexpr AlignedVec4f clamp(const AlignedVec4f &v, const AlignedVec4f &lo, const AlignedVec4f &hi) { return AlignedVec4f(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w)); }
inline constexpr AlignedVec4f min(const AlignedVec4f &a, float s) { return min(a, AlignedVec4f(s)); }
inline constexpr AlignedVec4f max(const AlignedVec4f &a, float s) { return max(a, AlignedVec4f(s)); }
inline constexpr BVec4 epsilonEqual(const AlignedVec4f &a, const AlignedVec4f &b, float eps) { return BVec4(epsilonEqual(a.x, b.x, eps), epsilonEqual(a.y, b.y, eps), epsilonEqual(a.z, b.z, eps), epsilonEqual(a.w, b.w, eps)); }
inline constexpr BVec4 epsilonEqual(const AlignedVec4f &a, const AlignedVec4f &b, const AlignedVec4f &eps) { return BVec4(epsilonEqual(a.x, b.x, eps.x), epsilonEqual(a.y, b.y, eps.y), epsilonEqual(a.z, b.z, eps.z), epsilonEqual(a.w, b.w, eps.w)); }
inline constexpr BVec4 epsilonNotEqual(const AlignedVec4f &a, const AlignedVec4f &b, float eps) { return BVec4(epsilonNotEqual(a.x, b.x, eps), epsilonNotEqual(a.y, b.y, eps), epsilonNotEqual(a.z, b.z, eps), epsilonNotEqual(a.w, b.w, eps)); }
inline constexpr BVec4 isnan(const AlignedVec4f &a) { return BVec4(isnan(a.x), isnan(a.y), isnan(a.z), isnan(a.w)); }
inline BVec4 isinf(const AlignedVec4f &a) { return BVec4(isinf(a.x), isinf(a.y), isinf(a.z), isinf(a.w)); }
inline constexpr BVec4 isfinite(const AlignedVec4f &a) { return BVec4(isfinite(a.x), isfinite(a.y), isfinite(a.z), isfinite(a.w)); }
inline AlignedVec4f sin(const AlignedVec4f &a) { return AlignedVec4f((float)sin(a.x), (float)sin(a.y), (float)sin(a.z), (float)sin(a.w)); }
inline AlignedVec4f cos(const AlignedVec4f &a) { return AlignedVec4f((float)cos(a.x), (float)cos(a.y), (float)cos(a.z), (float)cos(a.w)); }
inline AlignedVec4f tan(const AlignedVec4f &a) { return AlignedVec4f((float)tan((float)a.x), (float)tan((float)a.y), (float)tan((float)a.z), (float)tan((float)a.w)); }
inline constexpr AlignedVec4f radians(const AlignedVec4f &a) { return AlignedVec4f((float)radians(a.x), (float)radians(a.y), (float)radians(a.z), (float)radians(a.w)); }
inline constexpr AlignedVec4f degrees(const AlignedVec4f &a) { return AlignedVec4f((float)degrees(a.x), (float)degrees(a.y), (float)degrees(a.z), (float)degrees(a.w)); }
inline AlignedVec4f sqrt(const AlignedVec4f &a) { return AlignedVec4f((float)sqrt(a.x), (float)sqrt(a.y), (float)sqrt(a.z), (float)sqrt(a.w)); }
inline AlignedVec4f pow(const AlignedVec4f &a, float e) { return AlignedVec4f((float)pow((float)a.x, (float)e), (float)pow((float)a.y, (float)e), (float)pow((float)a.z, (float)e), (float)pow((float)a.w, (float)e)); }
inline constexpr AlignedVec4f sign(const AlignedVec4f &a) { return AlignedVec4f(sign((float)a.x), sign((float)a.y), sign((float)a.z), sign((float)a.w)); }
inline AlignedVec4f mod(const AlignedVec4f &a, float b) { return AlignedVec4f(mod((float)a.x, (float)b), mod((float)a.y, (float)b), mod((float)a.z, (float)b), mod((float)a.w, (float)b)); }
inline AlignedVec4f mod(const AlignedVec4f &a, const AlignedVec4f &b) { return AlignedVec4f(mod(a.x, b.x), mod(a.y, b.y), mod(a.z, b.z), mod(a.w, b.w)); }
inline constexpr const float *value_ptr(const AlignedVec4f &v) { return &v.x; }
inline constexpr float *value_ptr(AlignedVec4f &v) { return &v.x; }

}
