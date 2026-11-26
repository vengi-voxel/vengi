/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec2.h"

namespace math {

struct Vec2u {
	union {
		struct { uint32_t x, y; };
		struct { uint32_t r, g; };
	};
	typedef Vec2u col_type;
	typedef uint32_t value_type;
	static constexpr int length() { return 2; }

	Vec2u() = default;
	constexpr Vec2u(uint32_t s) : x(s), y(s) {}
	constexpr Vec2u(uint32_t x_, uint32_t y_) : x(x_), y(y_) {}

	constexpr uint32_t &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		return x;
	}
	constexpr uint32_t operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		return x;
	}
	constexpr Vec2u &operator+=(const Vec2u &o) { x += o.x; y += o.y; return *this; }
	constexpr Vec2u &operator+=(uint32_t s) { x += s; y += s; return *this; }
	constexpr Vec2u &operator-=(const Vec2u &o) { x -= o.x; y -= o.y; return *this; }
	constexpr Vec2u &operator-=(uint32_t s) { x -= s; y -= s; return *this; }
	constexpr Vec2u &operator*=(const Vec2u &o) { x *= o.x; y *= o.y; return *this; }
	constexpr Vec2u &operator*=(uint32_t s) { x *= s; y *= s; return *this; }
	constexpr Vec2u &operator/=(const Vec2u &o) { x /= o.x; y /= o.y; return *this; }
	constexpr Vec2u &operator/=(uint32_t s) { x /= s; y /= s; return *this; }
};

inline constexpr Vec2u operator+(const Vec2u &a, const Vec2u &o) { return Vec2u(a.x + o.x, a.y + o.y); }
inline constexpr Vec2u operator+(const Vec2u &a, uint32_t s) { return Vec2u(a.x + s, a.y + s); }
inline constexpr Vec2u operator+(uint32_t s, const Vec2u &a) { return Vec2u(s + a.x, s + a.y); }
inline constexpr Vec2u operator-(const Vec2u &a, const Vec2u &o) { return Vec2u(a.x - o.x, a.y - o.y); }
inline constexpr Vec2u operator-(const Vec2u &a, uint32_t s) { return Vec2u(a.x - s, a.y - s); }
inline constexpr Vec2u operator-(uint32_t s, const Vec2u &a) { return Vec2u(s - a.x, s - a.y); }
inline constexpr Vec2u operator*(const Vec2u &a, const Vec2u &o) { return Vec2u(a.x * o.x, a.y * o.y); }
inline constexpr Vec2u operator*(const Vec2u &a, uint32_t s) { return Vec2u(a.x * s, a.y * s); }
inline constexpr Vec2u operator*(uint32_t s, const Vec2u &a) { return Vec2u(s * a.x, s * a.y); }
inline constexpr Vec2u operator/(const Vec2u &a, const Vec2u &o) { return Vec2u(a.x / o.x, a.y / o.y); }
inline constexpr Vec2u operator/(const Vec2u &a, uint32_t s) { return Vec2u(a.x / s, a.y / s); }
inline constexpr Vec2u operator/(uint32_t s, const Vec2u &a) { return Vec2u(s / a.x, s / a.y); }
inline constexpr Vec2u operator%(const Vec2u &a, const Vec2u &o) { return Vec2u(a.x % o.x, a.y % o.y); }
inline constexpr Vec2u operator%(const Vec2u &a, uint32_t s) { return Vec2u(a.x % s, a.y % s); }
inline constexpr Vec2u operator%(uint32_t s, const Vec2u &a) { return Vec2u(s % a.x, s % a.y); }
inline constexpr Vec2u operator-(const Vec2u &a) { return Vec2u(0u - a.x, 0u - a.y); }
inline constexpr bool operator==(const Vec2u &a, const Vec2u &o) { return a.x == o.x && a.y == o.y; }
inline constexpr bool operator!=(const Vec2u &a, const Vec2u &o) { return !(a == o); }
inline constexpr uint32_t dot(const Vec2u &a, const Vec2u &b) { return a.x * b.x + a.y * b.y; }
inline constexpr Vec2u min(const Vec2u &a, const Vec2u &b) { return Vec2u(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y); }
inline constexpr Vec2u max(const Vec2u &a, const Vec2u &b) { return Vec2u(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y); }
inline constexpr Vec2u abs(const Vec2u &a) { return a; }
inline constexpr BVec2 lessThan(const Vec2u &a, const Vec2u &b) { return BVec2(a.x < b.x, a.y < b.y); }
inline constexpr BVec2 lessThanEqual(const Vec2u &a, const Vec2u &b) { return BVec2(a.x <= b.x, a.y <= b.y); }
inline constexpr BVec2 greaterThan(const Vec2u &a, const Vec2u &b) { return BVec2(a.x > b.x, a.y > b.y); }
inline constexpr BVec2 greaterThanEqual(const Vec2u &a, const Vec2u &b) { return BVec2(a.x >= b.x, a.y >= b.y); }
inline constexpr BVec2 equal(const Vec2u &a, const Vec2u &b) { return BVec2(a.x == b.x, a.y == b.y); }

inline constexpr Vec2u clamp(const Vec2u &v, const Vec2u &lo, const Vec2u &hi) { return Vec2u(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y)); }
inline constexpr Vec2u clamp(const Vec2u &v, uint32_t lo, uint32_t hi) { return Vec2u(clamp(v.x, lo, hi), clamp(v.y, lo, hi)); }
inline constexpr Vec2u min(const Vec2u &a, uint32_t s) { return min(a, Vec2u(s)); }
inline constexpr Vec2u max(const Vec2u &a, uint32_t s) { return max(a, Vec2u(s)); }
inline constexpr const uint32_t *value_ptr(const Vec2u &v) { return &v.x; }
inline constexpr uint32_t *value_ptr(Vec2u &v) { return &v.x; }

}
