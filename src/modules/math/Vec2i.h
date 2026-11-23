/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec2.h"

namespace math {
struct Vec2f;

struct Vec2i {
	union {
		struct { int x, y; };
		struct { int r, g; };
	};
	typedef Vec2i col_type;
	typedef int value_type;
	static constexpr int length() { return 2; }

	Vec2i() = default;
	constexpr Vec2i(int s) : x(s), y(s) {}
	constexpr Vec2i(int x_, int y_) : x(x_), y(y_) {}

	constexpr int &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		return x;
	}
	constexpr int operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		return x;
	}
	constexpr Vec2i(const Vec2f &v);
	constexpr Vec2i &operator+=(const Vec2i &o) { x += o.x; y += o.y; return *this; }
	constexpr Vec2i &operator+=(int s) { x += s; y += s; return *this; }
	constexpr Vec2i &operator-=(const Vec2i &o) { x -= o.x; y -= o.y; return *this; }
	constexpr Vec2i &operator-=(int s) { x -= s; y -= s; return *this; }
	constexpr Vec2i &operator*=(const Vec2i &o) { x *= o.x; y *= o.y; return *this; }
	constexpr Vec2i &operator*=(int s) { x *= s; y *= s; return *this; }
	constexpr Vec2i &operator/=(const Vec2i &o) { x /= o.x; y /= o.y; return *this; }
	constexpr Vec2i &operator/=(int s) { x /= s; y /= s; return *this; }
};

inline constexpr Vec2i operator+(const Vec2i &a, const Vec2i &o) { return Vec2i(a.x + o.x, a.y + o.y); }
inline constexpr Vec2i operator+(const Vec2i &a, int s) { return Vec2i(a.x + s, a.y + s); }
inline constexpr Vec2i operator+(int s, const Vec2i &a) { return Vec2i(s + a.x, s + a.y); }
inline constexpr Vec2i operator-(const Vec2i &a, const Vec2i &o) { return Vec2i(a.x - o.x, a.y - o.y); }
inline constexpr Vec2i operator-(const Vec2i &a, int s) { return Vec2i(a.x - s, a.y - s); }
inline constexpr Vec2i operator-(int s, const Vec2i &a) { return Vec2i(s - a.x, s - a.y); }
inline constexpr Vec2i operator*(const Vec2i &a, const Vec2i &o) { return Vec2i(a.x * o.x, a.y * o.y); }
inline constexpr Vec2i operator*(const Vec2i &a, int s) { return Vec2i(a.x * s, a.y * s); }
inline constexpr Vec2i operator*(int s, const Vec2i &a) { return Vec2i(s * a.x, s * a.y); }
inline constexpr Vec2i operator/(const Vec2i &a, const Vec2i &o) { return Vec2i(a.x / o.x, a.y / o.y); }
inline constexpr Vec2i operator/(const Vec2i &a, int s) { return Vec2i(a.x / s, a.y / s); }
inline constexpr Vec2i operator/(int s, const Vec2i &a) { return Vec2i(s / a.x, s / a.y); }
inline constexpr Vec2i operator%(const Vec2i &a, const Vec2i &o) { return Vec2i(a.x % o.x, a.y % o.y); }
inline constexpr Vec2i operator%(const Vec2i &a, int s) { return Vec2i(a.x % s, a.y % s); }
inline constexpr Vec2i operator%(int s, const Vec2i &a) { return Vec2i(s % a.x, s % a.y); }
inline constexpr Vec2i operator-(const Vec2i &a) { return Vec2i(-a.x, -a.y); }
inline constexpr bool operator==(const Vec2i &a, const Vec2i &o) { return a.x == o.x && a.y == o.y; }
inline constexpr bool operator!=(const Vec2i &a, const Vec2i &o) { return !(a == o); }
inline constexpr Vec2i operator&(const Vec2i &a, const Vec2i &o) { return Vec2i(a.x & o.x, a.y & o.y); }
inline constexpr Vec2i operator&(const Vec2i &a, int s) { return Vec2i(a.x & s, a.y & s); }
inline constexpr Vec2i operator|(const Vec2i &a, const Vec2i &o) { return Vec2i(a.x | o.x, a.y | o.y); }
inline constexpr Vec2i operator|(const Vec2i &a, int s) { return Vec2i(a.x | s, a.y | s); }
inline constexpr Vec2i operator^(const Vec2i &a, const Vec2i &o) { return Vec2i(a.x ^ o.x, a.y ^ o.y); }
inline constexpr Vec2i operator^(const Vec2i &a, int s) { return Vec2i(a.x ^ s, a.y ^ s); }
inline constexpr Vec2i operator~(const Vec2i &a) { return Vec2i(~a.x, ~a.y); }
inline Vec2i operator<<(const Vec2i &a, int s) { return Vec2i(a.x << s, a.y << s); }
inline constexpr Vec2i operator>>(const Vec2i &a, int s) { return Vec2i(a.x >> s, a.y >> s); }
inline constexpr int dot(const Vec2i &a, const Vec2i &b) { return a.x * b.x + a.y * b.y; }
inline constexpr Vec2i min(const Vec2i &a, const Vec2i &b) { return Vec2i(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y); }
inline constexpr Vec2i max(const Vec2i &a, const Vec2i &b) { return Vec2i(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y); }
inline constexpr Vec2i abs(const Vec2i &a) { return Vec2i(a.x < 0 ? -a.x : a.x, a.y < 0 ? -a.y : a.y); }
inline constexpr BVec2 lessThan(const Vec2i &a, const Vec2i &b) { return BVec2(a.x < b.x, a.y < b.y); }
inline constexpr BVec2 lessThanEqual(const Vec2i &a, const Vec2i &b) { return BVec2(a.x <= b.x, a.y <= b.y); }
inline constexpr BVec2 greaterThan(const Vec2i &a, const Vec2i &b) { return BVec2(a.x > b.x, a.y > b.y); }
inline constexpr BVec2 greaterThanEqual(const Vec2i &a, const Vec2i &b) { return BVec2(a.x >= b.x, a.y >= b.y); }
inline constexpr BVec2 equal(const Vec2i &a, const Vec2i &b) { return BVec2(a.x == b.x, a.y == b.y); }

inline constexpr Vec2i clamp(const Vec2i &v, const Vec2i &lo, const Vec2i &hi) { return Vec2i(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y)); }
inline constexpr Vec2i clamp(const Vec2i &v, int lo, int hi) { return Vec2i(clamp(v.x, lo, hi), clamp(v.y, lo, hi)); }
inline constexpr Vec2i min(const Vec2i &a, int s) { return min(a, Vec2i(s)); }
inline constexpr Vec2i max(const Vec2i &a, int s) { return max(a, Vec2i(s)); }
inline constexpr const int *value_ptr(const Vec2i &v) { return &v.x; }
inline constexpr int *value_ptr(Vec2i &v) { return &v.x; }

}
