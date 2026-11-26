/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec3.h"

namespace math {

struct Vec3u {
	union {
		struct { uint32_t x, y, z; };
		struct { uint32_t r, g, b; };
	};
	typedef Vec3u col_type;
	typedef uint32_t value_type;
	static constexpr int length() { return 3; }

	Vec3u() = default;
	constexpr Vec3u(uint32_t s) : x(s), y(s), z(s) {}
	constexpr Vec3u(uint32_t x_, uint32_t y_, uint32_t z_) : x(x_), y(y_), z(z_) {}

	constexpr uint32_t &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr uint32_t operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr Vec3u &operator+=(const Vec3u &o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr Vec3u &operator+=(uint32_t s) { x += s; y += s; z += s; return *this; }
	constexpr Vec3u &operator-=(const Vec3u &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr Vec3u &operator-=(uint32_t s) { x -= s; y -= s; z -= s; return *this; }
	constexpr Vec3u &operator*=(const Vec3u &o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr Vec3u &operator*=(uint32_t s) { x *= s; y *= s; z *= s; return *this; }
	constexpr Vec3u &operator/=(const Vec3u &o) { x /= o.x; y /= o.y; z /= o.z; return *this; }
	constexpr Vec3u &operator/=(uint32_t s) { x /= s; y /= s; z /= s; return *this; }
};

inline constexpr Vec3u operator+(const Vec3u &a, const Vec3u &o) { return Vec3u(a.x + o.x, a.y + o.y, a.z + o.z); }
inline constexpr Vec3u operator+(const Vec3u &a, uint32_t s) { return Vec3u(a.x + s, a.y + s, a.z + s); }
inline constexpr Vec3u operator+(uint32_t s, const Vec3u &a) { return Vec3u(s + a.x, s + a.y, s + a.z); }
inline constexpr Vec3u operator-(const Vec3u &a, const Vec3u &o) { return Vec3u(a.x - o.x, a.y - o.y, a.z - o.z); }
inline constexpr Vec3u operator-(const Vec3u &a, uint32_t s) { return Vec3u(a.x - s, a.y - s, a.z - s); }
inline constexpr Vec3u operator-(uint32_t s, const Vec3u &a) { return Vec3u(s - a.x, s - a.y, s - a.z); }
inline constexpr Vec3u operator*(const Vec3u &a, const Vec3u &o) { return Vec3u(a.x * o.x, a.y * o.y, a.z * o.z); }
inline constexpr Vec3u operator*(const Vec3u &a, uint32_t s) { return Vec3u(a.x * s, a.y * s, a.z * s); }
inline constexpr Vec3u operator*(uint32_t s, const Vec3u &a) { return Vec3u(s * a.x, s * a.y, s * a.z); }
inline constexpr Vec3u operator/(const Vec3u &a, const Vec3u &o) { return Vec3u(a.x / o.x, a.y / o.y, a.z / o.z); }
inline constexpr Vec3u operator/(const Vec3u &a, uint32_t s) { return Vec3u(a.x / s, a.y / s, a.z / s); }
inline constexpr Vec3u operator/(uint32_t s, const Vec3u &a) { return Vec3u(s / a.x, s / a.y, s / a.z); }
inline constexpr Vec3u operator%(const Vec3u &a, const Vec3u &o) { return Vec3u(a.x % o.x, a.y % o.y, a.z % o.z); }
inline constexpr Vec3u operator%(const Vec3u &a, uint32_t s) { return Vec3u(a.x % s, a.y % s, a.z % s); }
inline constexpr Vec3u operator%(uint32_t s, const Vec3u &a) { return Vec3u(s % a.x, s % a.y, s % a.z); }
inline constexpr Vec3u operator-(const Vec3u &a) { return Vec3u(0u - a.x, 0u - a.y, 0u - a.z); }
inline constexpr bool operator==(const Vec3u &a, const Vec3u &o) { return a.x == o.x && a.y == o.y && a.z == o.z; }
inline constexpr bool operator!=(const Vec3u &a, const Vec3u &o) { return !(a == o); }
inline constexpr uint32_t dot(const Vec3u &a, const Vec3u &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr Vec3u min(const Vec3u &a, const Vec3u &b) { return Vec3u(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z); }
inline constexpr Vec3u max(const Vec3u &a, const Vec3u &b) { return Vec3u(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z); }
inline constexpr Vec3u abs(const Vec3u &a) { return a; }
inline constexpr BVec3 lessThan(const Vec3u &a, const Vec3u &b) { return BVec3(a.x < b.x, a.y < b.y, a.z < b.z); }
inline constexpr BVec3 lessThanEqual(const Vec3u &a, const Vec3u &b) { return BVec3(a.x <= b.x, a.y <= b.y, a.z <= b.z); }
inline constexpr BVec3 greaterThan(const Vec3u &a, const Vec3u &b) { return BVec3(a.x > b.x, a.y > b.y, a.z > b.z); }
inline constexpr BVec3 greaterThanEqual(const Vec3u &a, const Vec3u &b) { return BVec3(a.x >= b.x, a.y >= b.y, a.z >= b.z); }
inline constexpr BVec3 equal(const Vec3u &a, const Vec3u &b) { return BVec3(a.x == b.x, a.y == b.y, a.z == b.z); }

inline constexpr Vec3u clamp(const Vec3u &v, const Vec3u &lo, const Vec3u &hi) { return Vec3u(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z)); }
inline constexpr Vec3u clamp(const Vec3u &v, uint32_t lo, uint32_t hi) { return Vec3u(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi)); }
inline constexpr Vec3u min(const Vec3u &a, uint32_t s) { return min(a, Vec3u(s)); }
inline constexpr Vec3u max(const Vec3u &a, uint32_t s) { return max(a, Vec3u(s)); }
inline constexpr const uint32_t *value_ptr(const Vec3u &v) { return &v.x; }
inline constexpr uint32_t *value_ptr(Vec3u &v) { return &v.x; }

}
