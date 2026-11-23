/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec3.h"

namespace math {
struct Vec3f;
struct Vec2i;
struct AlignedVec4i;

struct Vec3i {
	union {
		struct { int x, y, z; };
		struct { int r, g, b; };
	};
	typedef Vec3i col_type;
	typedef int value_type;
	static constexpr int length() { return 3; }

	Vec3i() = default;
	constexpr Vec3i(int s) : x(s), y(s), z(s) {}
	constexpr Vec3i(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}

	constexpr int &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr int operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr Vec3i(const Vec3f &v);
	Vec3i(const Vec2i &v, int z_);
	constexpr Vec3i(const AlignedVec4i &v);
	constexpr Vec3i &operator+=(const Vec3i &o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr Vec3i &operator+=(int s) { x += s; y += s; z += s; return *this; }
	constexpr Vec3i &operator-=(const Vec3i &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr Vec3i &operator-=(int s) { x -= s; y -= s; z -= s; return *this; }
	constexpr Vec3i &operator*=(const Vec3i &o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr Vec3i &operator*=(int s) { x *= s; y *= s; z *= s; return *this; }
	constexpr Vec3i &operator/=(const Vec3i &o) { x /= o.x; y /= o.y; z /= o.z; return *this; }
	constexpr Vec3i &operator/=(int s) { x /= s; y /= s; z /= s; return *this; }
};

inline constexpr Vec3i operator+(const Vec3i &a, const Vec3i &o) { return Vec3i(a.x + o.x, a.y + o.y, a.z + o.z); }
inline constexpr Vec3i operator+(const Vec3i &a, int s) { return Vec3i(a.x + s, a.y + s, a.z + s); }
inline constexpr Vec3i operator+(int s, const Vec3i &a) { return Vec3i(s + a.x, s + a.y, s + a.z); }
inline constexpr Vec3i operator-(const Vec3i &a, const Vec3i &o) { return Vec3i(a.x - o.x, a.y - o.y, a.z - o.z); }
inline constexpr Vec3i operator-(const Vec3i &a, int s) { return Vec3i(a.x - s, a.y - s, a.z - s); }
inline constexpr Vec3i operator-(int s, const Vec3i &a) { return Vec3i(s - a.x, s - a.y, s - a.z); }
inline constexpr Vec3i operator*(const Vec3i &a, const Vec3i &o) { return Vec3i(a.x * o.x, a.y * o.y, a.z * o.z); }
inline constexpr Vec3i operator*(const Vec3i &a, int s) { return Vec3i(a.x * s, a.y * s, a.z * s); }
inline constexpr Vec3i operator*(int s, const Vec3i &a) { return Vec3i(s * a.x, s * a.y, s * a.z); }
inline constexpr Vec3i operator/(const Vec3i &a, const Vec3i &o) { return Vec3i(a.x / o.x, a.y / o.y, a.z / o.z); }
inline constexpr Vec3i operator/(const Vec3i &a, int s) { return Vec3i(a.x / s, a.y / s, a.z / s); }
inline constexpr Vec3i operator/(int s, const Vec3i &a) { return Vec3i(s / a.x, s / a.y, s / a.z); }
inline constexpr Vec3i operator%(const Vec3i &a, const Vec3i &o) { return Vec3i(a.x % o.x, a.y % o.y, a.z % o.z); }
inline constexpr Vec3i operator%(const Vec3i &a, int s) { return Vec3i(a.x % s, a.y % s, a.z % s); }
inline constexpr Vec3i operator%(int s, const Vec3i &a) { return Vec3i(s % a.x, s % a.y, s % a.z); }
inline constexpr Vec3i operator-(const Vec3i &a) { return Vec3i(-a.x, -a.y, -a.z); }
inline constexpr bool operator==(const Vec3i &a, const Vec3i &o) { return a.x == o.x && a.y == o.y && a.z == o.z; }
inline constexpr bool operator!=(const Vec3i &a, const Vec3i &o) { return !(a == o); }
inline constexpr Vec3i operator&(const Vec3i &a, const Vec3i &o) { return Vec3i(a.x & o.x, a.y & o.y, a.z & o.z); }
inline constexpr Vec3i operator&(const Vec3i &a, int s) { return Vec3i(a.x & s, a.y & s, a.z & s); }
inline constexpr Vec3i operator|(const Vec3i &a, const Vec3i &o) { return Vec3i(a.x | o.x, a.y | o.y, a.z | o.z); }
inline constexpr Vec3i operator|(const Vec3i &a, int s) { return Vec3i(a.x | s, a.y | s, a.z | s); }
inline constexpr Vec3i operator^(const Vec3i &a, const Vec3i &o) { return Vec3i(a.x ^ o.x, a.y ^ o.y, a.z ^ o.z); }
inline constexpr Vec3i operator^(const Vec3i &a, int s) { return Vec3i(a.x ^ s, a.y ^ s, a.z ^ s); }
inline constexpr Vec3i operator~(const Vec3i &a) { return Vec3i(~a.x, ~a.y, ~a.z); }
inline Vec3i operator<<(const Vec3i &a, int s) { return Vec3i(a.x << s, a.y << s, a.z << s); }
inline constexpr Vec3i operator>>(const Vec3i &a, int s) { return Vec3i(a.x >> s, a.y >> s, a.z >> s); }
inline constexpr int dot(const Vec3i &a, const Vec3i &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr Vec3i min(const Vec3i &a, const Vec3i &b) { return Vec3i(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z); }
inline constexpr Vec3i max(const Vec3i &a, const Vec3i &b) { return Vec3i(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z); }
inline constexpr Vec3i abs(const Vec3i &a) { return Vec3i(a.x < 0 ? -a.x : a.x, a.y < 0 ? -a.y : a.y, a.z < 0 ? -a.z : a.z); }
inline constexpr BVec3 lessThan(const Vec3i &a, const Vec3i &b) { return BVec3(a.x < b.x, a.y < b.y, a.z < b.z); }
inline constexpr BVec3 lessThanEqual(const Vec3i &a, const Vec3i &b) { return BVec3(a.x <= b.x, a.y <= b.y, a.z <= b.z); }
inline constexpr BVec3 greaterThan(const Vec3i &a, const Vec3i &b) { return BVec3(a.x > b.x, a.y > b.y, a.z > b.z); }
inline constexpr BVec3 greaterThanEqual(const Vec3i &a, const Vec3i &b) { return BVec3(a.x >= b.x, a.y >= b.y, a.z >= b.z); }
inline constexpr BVec3 equal(const Vec3i &a, const Vec3i &b) { return BVec3(a.x == b.x, a.y == b.y, a.z == b.z); }

inline constexpr Vec3i clamp(const Vec3i &v, const Vec3i &lo, const Vec3i &hi) { return Vec3i(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z)); }
inline constexpr Vec3i clamp(const Vec3i &v, int lo, int hi) { return Vec3i(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi)); }
inline constexpr Vec3i min(const Vec3i &a, int s) { return min(a, Vec3i(s)); }
inline constexpr Vec3i max(const Vec3i &a, int s) { return max(a, Vec3i(s)); }
inline constexpr const int *value_ptr(const Vec3i &v) { return &v.x; }
inline constexpr int *value_ptr(Vec3i &v) { return &v.x; }

struct Vec3iHash {
	constexpr uint32_t operator()(const Vec3i &v) const {
		uint32_t h = (uint32_t)v.x * 73856093u;
		h ^= (uint32_t)v.y * 19349663u;
		h ^= (uint32_t)v.z * 83492791u;
		return h;
	}
};

}
