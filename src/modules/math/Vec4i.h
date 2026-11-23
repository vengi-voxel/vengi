/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec4.h"

namespace math {
struct Vec3i;
struct Vec4f;
struct AlignedVec4i;

struct Vec4i {
	union {
		struct { int x, y, z, w; };
		struct { int r, g, b, a; };
	};
	typedef Vec4i col_type;
	typedef int value_type;
	static constexpr int length() { return 4; }

	Vec4i() = default;
	constexpr Vec4i(int s) : x(s), y(s), z(s), w(s) {}
	constexpr Vec4i(int x_, int y_, int z_, int w_) : x(x_), y(y_), z(z_), w(w_) {}

	constexpr int &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr int operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr Vec4i(const Vec3i &v, int w_);
	constexpr Vec4i(const Vec4f &v);
	constexpr Vec4i(const AlignedVec4i &v);
	constexpr Vec4i &operator+=(const Vec4i &o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr Vec4i &operator+=(int s) { x += s; y += s; z += s; w += s; return *this; }
	constexpr Vec4i &operator-=(const Vec4i &o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr Vec4i &operator-=(int s) { x -= s; y -= s; z -= s; w -= s; return *this; }
	constexpr Vec4i &operator*=(const Vec4i &o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr Vec4i &operator*=(int s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr Vec4i &operator/=(const Vec4i &o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
	constexpr Vec4i &operator/=(int s) { x /= s; y /= s; z /= s; w /= s; return *this; }
};

inline constexpr Vec4i operator+(const Vec4i &a, const Vec4i &o) { return Vec4i(a.x + o.x, a.y + o.y, a.z + o.z, a.w + o.w); }
inline constexpr Vec4i operator+(const Vec4i &a, int s) { return Vec4i(a.x + s, a.y + s, a.z + s, a.w + s); }
inline constexpr Vec4i operator+(int s, const Vec4i &a) { return Vec4i(s + a.x, s + a.y, s + a.z, s + a.w); }
inline constexpr Vec4i operator-(const Vec4i &a, const Vec4i &o) { return Vec4i(a.x - o.x, a.y - o.y, a.z - o.z, a.w - o.w); }
inline constexpr Vec4i operator-(const Vec4i &a, int s) { return Vec4i(a.x - s, a.y - s, a.z - s, a.w - s); }
inline constexpr Vec4i operator-(int s, const Vec4i &a) { return Vec4i(s - a.x, s - a.y, s - a.z, s - a.w); }
inline constexpr Vec4i operator*(const Vec4i &a, const Vec4i &o) { return Vec4i(a.x * o.x, a.y * o.y, a.z * o.z, a.w * o.w); }
inline constexpr Vec4i operator*(const Vec4i &a, int s) { return Vec4i(a.x * s, a.y * s, a.z * s, a.w * s); }
inline constexpr Vec4i operator*(int s, const Vec4i &a) { return Vec4i(s * a.x, s * a.y, s * a.z, s * a.w); }
inline constexpr Vec4i operator/(const Vec4i &a, const Vec4i &o) { return Vec4i(a.x / o.x, a.y / o.y, a.z / o.z, a.w / o.w); }
inline constexpr Vec4i operator/(const Vec4i &a, int s) { return Vec4i(a.x / s, a.y / s, a.z / s, a.w / s); }
inline constexpr Vec4i operator/(int s, const Vec4i &a) { return Vec4i(s / a.x, s / a.y, s / a.z, s / a.w); }
inline constexpr Vec4i operator%(const Vec4i &a, const Vec4i &o) { return Vec4i(a.x % o.x, a.y % o.y, a.z % o.z, a.w % o.w); }
inline constexpr Vec4i operator%(const Vec4i &a, int s) { return Vec4i(a.x % s, a.y % s, a.z % s, a.w % s); }
inline constexpr Vec4i operator%(int s, const Vec4i &a) { return Vec4i(s % a.x, s % a.y, s % a.z, s % a.w); }
inline constexpr Vec4i operator-(const Vec4i &a) { return Vec4i(-a.x, -a.y, -a.z, -a.w); }
inline constexpr bool operator==(const Vec4i &a, const Vec4i &o) { return a.x == o.x && a.y == o.y && a.z == o.z && a.w == o.w; }
inline constexpr bool operator!=(const Vec4i &a, const Vec4i &o) { return !(a == o); }
inline constexpr Vec4i operator&(const Vec4i &a, const Vec4i &o) { return Vec4i(a.x & o.x, a.y & o.y, a.z & o.z, a.w & o.w); }
inline constexpr Vec4i operator&(const Vec4i &a, int s) { return Vec4i(a.x & s, a.y & s, a.z & s, a.w & s); }
inline constexpr Vec4i operator|(const Vec4i &a, const Vec4i &o) { return Vec4i(a.x | o.x, a.y | o.y, a.z | o.z, a.w | o.w); }
inline constexpr Vec4i operator|(const Vec4i &a, int s) { return Vec4i(a.x | s, a.y | s, a.z | s, a.w | s); }
inline constexpr Vec4i operator^(const Vec4i &a, const Vec4i &o) { return Vec4i(a.x ^ o.x, a.y ^ o.y, a.z ^ o.z, a.w ^ o.w); }
inline constexpr Vec4i operator^(const Vec4i &a, int s) { return Vec4i(a.x ^ s, a.y ^ s, a.z ^ s, a.w ^ s); }
inline constexpr Vec4i operator~(const Vec4i &a) { return Vec4i(~a.x, ~a.y, ~a.z, ~a.w); }
inline Vec4i operator<<(const Vec4i &a, int s) { return Vec4i(a.x << s, a.y << s, a.z << s, a.w << s); }
inline constexpr Vec4i operator>>(const Vec4i &a, int s) { return Vec4i(a.x >> s, a.y >> s, a.z >> s, a.w >> s); }
inline constexpr int dot(const Vec4i &a, const Vec4i &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline constexpr Vec4i min(const Vec4i &a, const Vec4i &b) { return Vec4i(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w); }
inline constexpr Vec4i max(const Vec4i &a, const Vec4i &b) { return Vec4i(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w); }
inline constexpr Vec4i abs(const Vec4i &a) { return Vec4i(a.x < 0 ? -a.x : a.x, a.y < 0 ? -a.y : a.y, a.z < 0 ? -a.z : a.z, a.w < 0 ? -a.w : a.w); }
inline constexpr BVec4 lessThan(const Vec4i &a, const Vec4i &b) { return BVec4(a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w); }
inline constexpr BVec4 lessThanEqual(const Vec4i &a, const Vec4i &b) { return BVec4(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); }
inline constexpr BVec4 greaterThan(const Vec4i &a, const Vec4i &b) { return BVec4(a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w); }
inline constexpr BVec4 greaterThanEqual(const Vec4i &a, const Vec4i &b) { return BVec4(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }
inline constexpr BVec4 equal(const Vec4i &a, const Vec4i &b) { return BVec4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); }

inline constexpr Vec4i clamp(const Vec4i &v, const Vec4i &lo, const Vec4i &hi) { return Vec4i(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w)); }
inline constexpr Vec4i clamp(const Vec4i &v, int lo, int hi) { return Vec4i(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi)); }
inline constexpr Vec4i min(const Vec4i &a, int s) { return min(a, Vec4i(s)); }
inline constexpr Vec4i max(const Vec4i &a, int s) { return max(a, Vec4i(s)); }
inline constexpr const int *value_ptr(const Vec4i &v) { return &v.x; }
inline constexpr int *value_ptr(Vec4i &v) { return &v.x; }

struct Vec4iHash {
	constexpr uint32_t operator()(const Vec4i &v) const {
		uint32_t h = (uint32_t)v.x * 73856093u;
		h ^= (uint32_t)v.y * 19349663u;
		h ^= (uint32_t)v.z * 83492791u;
		h ^= (uint32_t)v.w * 50331653u;
		return h;
	}
};

}
