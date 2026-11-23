/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec3.h"

namespace math {

struct U16Vec3 {
	union {
		struct { uint16_t x, y, z; };
		struct { uint16_t r, g, b; };
	};
	typedef U16Vec3 col_type;
	typedef uint16_t value_type;
	static constexpr int length() { return 3; }

	U16Vec3() = default;
	constexpr U16Vec3(uint16_t s) : x(s), y(s), z(s) {}
	constexpr U16Vec3(uint16_t x_, uint16_t y_, uint16_t z_) : x(x_), y(y_), z(z_) {}

	constexpr uint16_t &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr uint16_t operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr U16Vec3 &operator+=(const U16Vec3 &o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr U16Vec3 &operator+=(uint16_t s) { x += s; y += s; z += s; return *this; }
	constexpr U16Vec3 &operator-=(const U16Vec3 &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr U16Vec3 &operator-=(uint16_t s) { x -= s; y -= s; z -= s; return *this; }
	constexpr U16Vec3 &operator*=(const U16Vec3 &o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr U16Vec3 &operator*=(uint16_t s) { x *= s; y *= s; z *= s; return *this; }
	constexpr U16Vec3 &operator/=(const U16Vec3 &o) { x /= o.x; y /= o.y; z /= o.z; return *this; }
	constexpr U16Vec3 &operator/=(uint16_t s) { x /= s; y /= s; z /= s; return *this; }
};

inline constexpr U16Vec3 operator+(const U16Vec3 &a, const U16Vec3 &o) { return U16Vec3(a.x + o.x, a.y + o.y, a.z + o.z); }
inline constexpr U16Vec3 operator+(const U16Vec3 &a, uint16_t s) { return U16Vec3(a.x + s, a.y + s, a.z + s); }
inline constexpr U16Vec3 operator+(uint16_t s, const U16Vec3 &a) { return U16Vec3(s + a.x, s + a.y, s + a.z); }
inline constexpr U16Vec3 operator-(const U16Vec3 &a, const U16Vec3 &o) { return U16Vec3(a.x - o.x, a.y - o.y, a.z - o.z); }
inline constexpr U16Vec3 operator-(const U16Vec3 &a, uint16_t s) { return U16Vec3(a.x - s, a.y - s, a.z - s); }
inline constexpr U16Vec3 operator-(uint16_t s, const U16Vec3 &a) { return U16Vec3(s - a.x, s - a.y, s - a.z); }
inline constexpr U16Vec3 operator*(const U16Vec3 &a, const U16Vec3 &o) { return U16Vec3(a.x * o.x, a.y * o.y, a.z * o.z); }
inline constexpr U16Vec3 operator*(const U16Vec3 &a, uint16_t s) { return U16Vec3(a.x * s, a.y * s, a.z * s); }
inline constexpr U16Vec3 operator*(uint16_t s, const U16Vec3 &a) { return U16Vec3(s * a.x, s * a.y, s * a.z); }
inline constexpr U16Vec3 operator/(const U16Vec3 &a, const U16Vec3 &o) { return U16Vec3(a.x / o.x, a.y / o.y, a.z / o.z); }
inline constexpr U16Vec3 operator/(const U16Vec3 &a, uint16_t s) { return U16Vec3(a.x / s, a.y / s, a.z / s); }
inline constexpr U16Vec3 operator/(uint16_t s, const U16Vec3 &a) { return U16Vec3(s / a.x, s / a.y, s / a.z); }
inline constexpr U16Vec3 operator%(const U16Vec3 &a, const U16Vec3 &o) { return U16Vec3(a.x % o.x, a.y % o.y, a.z % o.z); }
inline constexpr U16Vec3 operator%(const U16Vec3 &a, uint16_t s) { return U16Vec3(a.x % s, a.y % s, a.z % s); }
inline constexpr U16Vec3 operator%(uint16_t s, const U16Vec3 &a) { return U16Vec3(s % a.x, s % a.y, s % a.z); }
inline constexpr U16Vec3 operator-(const U16Vec3 &a) { return U16Vec3(-a.x, -a.y, -a.z); }
inline constexpr bool operator==(const U16Vec3 &a, const U16Vec3 &o) { return a.x == o.x && a.y == o.y && a.z == o.z; }
inline constexpr bool operator!=(const U16Vec3 &a, const U16Vec3 &o) { return !(a == o); }
inline constexpr uint16_t dot(const U16Vec3 &a, const U16Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr U16Vec3 min(const U16Vec3 &a, const U16Vec3 &b) { return U16Vec3(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z); }
inline constexpr U16Vec3 max(const U16Vec3 &a, const U16Vec3 &b) { return U16Vec3(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z); }
inline constexpr U16Vec3 abs(const U16Vec3 &a) { return a; }
inline constexpr BVec3 lessThan(const U16Vec3 &a, const U16Vec3 &b) { return BVec3(a.x < b.x, a.y < b.y, a.z < b.z); }
inline constexpr BVec3 lessThanEqual(const U16Vec3 &a, const U16Vec3 &b) { return BVec3(a.x <= b.x, a.y <= b.y, a.z <= b.z); }
inline constexpr BVec3 greaterThan(const U16Vec3 &a, const U16Vec3 &b) { return BVec3(a.x > b.x, a.y > b.y, a.z > b.z); }
inline constexpr BVec3 greaterThanEqual(const U16Vec3 &a, const U16Vec3 &b) { return BVec3(a.x >= b.x, a.y >= b.y, a.z >= b.z); }
inline constexpr BVec3 equal(const U16Vec3 &a, const U16Vec3 &b) { return BVec3(a.x == b.x, a.y == b.y, a.z == b.z); }

inline constexpr U16Vec3 clamp(const U16Vec3 &v, const U16Vec3 &lo, const U16Vec3 &hi) { return U16Vec3(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z)); }
inline constexpr U16Vec3 clamp(const U16Vec3 &v, uint16_t lo, uint16_t hi) { return U16Vec3(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi)); }
inline constexpr U16Vec3 min(const U16Vec3 &a, uint16_t s) { return min(a, U16Vec3(s)); }
inline constexpr U16Vec3 max(const U16Vec3 &a, uint16_t s) { return max(a, U16Vec3(s)); }
inline constexpr const uint16_t *value_ptr(const U16Vec3 &v) { return &v.x; }
inline constexpr uint16_t *value_ptr(U16Vec3 &v) { return &v.x; }

}
