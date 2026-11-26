/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec4.h"

namespace math {

struct Vec4u {
	union {
		struct { uint32_t x, y, z, w; };
		struct { uint32_t r, g, b, a; };
	};
	typedef Vec4u col_type;
	typedef uint32_t value_type;
	static constexpr int length() { return 4; }

	Vec4u() = default;
	constexpr Vec4u(uint32_t s) : x(s), y(s), z(s), w(s) {}
	constexpr Vec4u(uint32_t x_, uint32_t y_, uint32_t z_, uint32_t w_) : x(x_), y(y_), z(z_), w(w_) {}

	constexpr uint32_t &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr uint32_t operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr Vec4u &operator+=(const Vec4u &o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr Vec4u &operator+=(uint32_t s) { x += s; y += s; z += s; w += s; return *this; }
	constexpr Vec4u &operator-=(const Vec4u &o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr Vec4u &operator-=(uint32_t s) { x -= s; y -= s; z -= s; w -= s; return *this; }
	constexpr Vec4u &operator*=(const Vec4u &o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr Vec4u &operator*=(uint32_t s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr Vec4u &operator/=(const Vec4u &o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
	constexpr Vec4u &operator/=(uint32_t s) { x /= s; y /= s; z /= s; w /= s; return *this; }
};

inline constexpr Vec4u operator+(const Vec4u &a, const Vec4u &o) { return Vec4u(a.x + o.x, a.y + o.y, a.z + o.z, a.w + o.w); }
inline constexpr Vec4u operator+(const Vec4u &a, uint32_t s) { return Vec4u(a.x + s, a.y + s, a.z + s, a.w + s); }
inline constexpr Vec4u operator+(uint32_t s, const Vec4u &a) { return Vec4u(s + a.x, s + a.y, s + a.z, s + a.w); }
inline constexpr Vec4u operator-(const Vec4u &a, const Vec4u &o) { return Vec4u(a.x - o.x, a.y - o.y, a.z - o.z, a.w - o.w); }
inline constexpr Vec4u operator-(const Vec4u &a, uint32_t s) { return Vec4u(a.x - s, a.y - s, a.z - s, a.w - s); }
inline constexpr Vec4u operator-(uint32_t s, const Vec4u &a) { return Vec4u(s - a.x, s - a.y, s - a.z, s - a.w); }
inline constexpr Vec4u operator*(const Vec4u &a, const Vec4u &o) { return Vec4u(a.x * o.x, a.y * o.y, a.z * o.z, a.w * o.w); }
inline constexpr Vec4u operator*(const Vec4u &a, uint32_t s) { return Vec4u(a.x * s, a.y * s, a.z * s, a.w * s); }
inline constexpr Vec4u operator*(uint32_t s, const Vec4u &a) { return Vec4u(s * a.x, s * a.y, s * a.z, s * a.w); }
inline constexpr Vec4u operator/(const Vec4u &a, const Vec4u &o) { return Vec4u(a.x / o.x, a.y / o.y, a.z / o.z, a.w / o.w); }
inline constexpr Vec4u operator/(const Vec4u &a, uint32_t s) { return Vec4u(a.x / s, a.y / s, a.z / s, a.w / s); }
inline constexpr Vec4u operator/(uint32_t s, const Vec4u &a) { return Vec4u(s / a.x, s / a.y, s / a.z, s / a.w); }
inline constexpr Vec4u operator%(const Vec4u &a, const Vec4u &o) { return Vec4u(a.x % o.x, a.y % o.y, a.z % o.z, a.w % o.w); }
inline constexpr Vec4u operator%(const Vec4u &a, uint32_t s) { return Vec4u(a.x % s, a.y % s, a.z % s, a.w % s); }
inline constexpr Vec4u operator%(uint32_t s, const Vec4u &a) { return Vec4u(s % a.x, s % a.y, s % a.z, s % a.w); }
inline constexpr Vec4u operator-(const Vec4u &a) { return Vec4u(0u - a.x, 0u - a.y, 0u - a.z, 0u - a.w); }
inline constexpr bool operator==(const Vec4u &a, const Vec4u &o) { return a.x == o.x && a.y == o.y && a.z == o.z && a.w == o.w; }
inline constexpr bool operator!=(const Vec4u &a, const Vec4u &o) { return !(a == o); }
inline constexpr uint32_t dot(const Vec4u &a, const Vec4u &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline constexpr Vec4u min(const Vec4u &a, const Vec4u &b) { return Vec4u(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w); }
inline constexpr Vec4u max(const Vec4u &a, const Vec4u &b) { return Vec4u(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w); }
inline constexpr Vec4u abs(const Vec4u &a) { return a; }
inline constexpr BVec4 lessThan(const Vec4u &a, const Vec4u &b) { return BVec4(a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w); }
inline constexpr BVec4 lessThanEqual(const Vec4u &a, const Vec4u &b) { return BVec4(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); }
inline constexpr BVec4 greaterThan(const Vec4u &a, const Vec4u &b) { return BVec4(a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w); }
inline constexpr BVec4 greaterThanEqual(const Vec4u &a, const Vec4u &b) { return BVec4(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }
inline constexpr BVec4 equal(const Vec4u &a, const Vec4u &b) { return BVec4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); }

inline constexpr Vec4u clamp(const Vec4u &v, const Vec4u &lo, const Vec4u &hi) { return Vec4u(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w)); }
inline constexpr Vec4u clamp(const Vec4u &v, uint32_t lo, uint32_t hi) { return Vec4u(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi)); }
inline constexpr Vec4u min(const Vec4u &a, uint32_t s) { return min(a, Vec4u(s)); }
inline constexpr Vec4u max(const Vec4u &a, uint32_t s) { return max(a, Vec4u(s)); }
inline constexpr const uint32_t *value_ptr(const Vec4u &v) { return &v.x; }
inline constexpr uint32_t *value_ptr(Vec4u &v) { return &v.x; }

}
