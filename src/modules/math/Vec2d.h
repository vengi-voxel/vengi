/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec2.h"

namespace math {

struct Vec2d {
	union {
		struct { double x, y; };
		struct { double r, g; };
	};
	typedef Vec2d col_type;
	typedef double value_type;
	static constexpr int length() { return 2; }

	Vec2d() = default;
	constexpr Vec2d(double s) : x(s), y(s) {}
	constexpr Vec2d(double x_, double y_) : x(x_), y(y_) {}

	constexpr double &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		return x;
	}
	constexpr double operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		return x;
	}
	constexpr Vec2d &operator+=(const Vec2d &o) { x += o.x; y += o.y; return *this; }
	constexpr Vec2d &operator+=(double s) { x += s; y += s; return *this; }
	constexpr Vec2d &operator-=(const Vec2d &o) { x -= o.x; y -= o.y; return *this; }
	constexpr Vec2d &operator-=(double s) { x -= s; y -= s; return *this; }
	constexpr Vec2d &operator*=(const Vec2d &o) { x *= o.x; y *= o.y; return *this; }
	constexpr Vec2d &operator*=(double s) { x *= s; y *= s; return *this; }
	constexpr Vec2d &operator/=(const Vec2d &o) { x /= o.x; y /= o.y; return *this; }
	constexpr Vec2d &operator/=(double s) { x /= s; y /= s; return *this; }
};

inline constexpr Vec2d operator+(const Vec2d &a, const Vec2d &o) { return Vec2d(a.x + o.x, a.y + o.y); }
inline constexpr Vec2d operator+(const Vec2d &a, double s) { return Vec2d(a.x + s, a.y + s); }
inline constexpr Vec2d operator+(double s, const Vec2d &a) { return Vec2d(s + a.x, s + a.y); }
inline constexpr Vec2d operator-(const Vec2d &a, const Vec2d &o) { return Vec2d(a.x - o.x, a.y - o.y); }
inline constexpr Vec2d operator-(const Vec2d &a, double s) { return Vec2d(a.x - s, a.y - s); }
inline constexpr Vec2d operator-(double s, const Vec2d &a) { return Vec2d(s - a.x, s - a.y); }
inline constexpr Vec2d operator*(const Vec2d &a, const Vec2d &o) { return Vec2d(a.x * o.x, a.y * o.y); }
inline constexpr Vec2d operator*(const Vec2d &a, double s) { return Vec2d(a.x * s, a.y * s); }
inline constexpr Vec2d operator*(double s, const Vec2d &a) { return Vec2d(s * a.x, s * a.y); }
inline constexpr Vec2d operator/(const Vec2d &a, const Vec2d &o) { return Vec2d(a.x / o.x, a.y / o.y); }
inline constexpr Vec2d operator/(const Vec2d &a, double s) { return Vec2d(a.x / s, a.y / s); }
inline constexpr Vec2d operator/(double s, const Vec2d &a) { return Vec2d(s / a.x, s / a.y); }
inline constexpr Vec2d operator-(const Vec2d &a) { return Vec2d(-a.x, -a.y); }
inline constexpr bool operator==(const Vec2d &a, const Vec2d &o) { return a.x == o.x && a.y == o.y; }
inline constexpr bool operator!=(const Vec2d &a, const Vec2d &o) { return !(a == o); }
inline constexpr double dot(const Vec2d &a, const Vec2d &b) { return a.x * b.x + a.y * b.y; }
inline constexpr Vec2d min(const Vec2d &a, const Vec2d &b) { return Vec2d(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y); }
inline constexpr Vec2d max(const Vec2d &a, const Vec2d &b) { return Vec2d(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y); }

inline constexpr BVec2 lessThan(const Vec2d &a, const Vec2d &b) { return BVec2(a.x < b.x, a.y < b.y); }
inline constexpr BVec2 lessThanEqual(const Vec2d &a, const Vec2d &b) { return BVec2(a.x <= b.x, a.y <= b.y); }
inline constexpr BVec2 greaterThan(const Vec2d &a, const Vec2d &b) { return BVec2(a.x > b.x, a.y > b.y); }
inline constexpr BVec2 greaterThanEqual(const Vec2d &a, const Vec2d &b) { return BVec2(a.x >= b.x, a.y >= b.y); }
inline constexpr BVec2 equal(const Vec2d &a, const Vec2d &b) { return BVec2(a.x == b.x, a.y == b.y); }

inline constexpr Vec2d abs(const Vec2d &a) { return Vec2d(abs(a.x), abs(a.y)); }
inline double length(const Vec2d &a) { return sqrt(dot(a, a)); }
inline constexpr double length2(const Vec2d &a) { return dot(a, a); }
inline double distance(const Vec2d &a, const Vec2d &b) { return length(a - b); }
inline constexpr double distance2(const Vec2d &a, const Vec2d &b) { return length2(a - b); }
inline Vec2d normalize(const Vec2d &a) { double len = length(a); return len > 0.0 ? a / len : Vec2d(0.0); }
inline Vec2d floor(const Vec2d &a) { return Vec2d(floor(a.x), floor(a.y)); }
inline Vec2d ceil(const Vec2d &a) { return Vec2d(ceil(a.x), ceil(a.y)); }
inline Vec2d round(const Vec2d &a) { return Vec2d(round(a.x), round(a.y)); }
inline Vec2d fract(const Vec2d &a) { return Vec2d(fract(a.x), fract(a.y)); }
inline constexpr Vec2d mix(const Vec2d &a, const Vec2d &b, double t) { return Vec2d(mix(a.x, b.x, t), mix(a.y, b.y, t)); }
inline constexpr Vec2d clamp(const Vec2d &v, double lo, double hi) { return Vec2d(clamp(v.x, lo, hi), clamp(v.y, lo, hi)); }
inline constexpr Vec2d clamp(const Vec2d &v, const Vec2d &lo, const Vec2d &hi) { return Vec2d(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y)); }
inline constexpr Vec2d min(const Vec2d &a, double s) { return min(a, Vec2d(s)); }
inline constexpr Vec2d max(const Vec2d &a, double s) { return max(a, Vec2d(s)); }
inline constexpr BVec2 epsilonEqual(const Vec2d &a, const Vec2d &b, double eps) { return BVec2(epsilonEqual(a.x, b.x, eps), epsilonEqual(a.y, b.y, eps)); }
inline constexpr BVec2 epsilonEqual(const Vec2d &a, const Vec2d &b, const Vec2d &eps) { return BVec2(epsilonEqual(a.x, b.x, eps.x), epsilonEqual(a.y, b.y, eps.y)); }
inline constexpr BVec2 epsilonNotEqual(const Vec2d &a, const Vec2d &b, double eps) { return BVec2(epsilonNotEqual(a.x, b.x, eps), epsilonNotEqual(a.y, b.y, eps)); }
inline constexpr BVec2 isnan(const Vec2d &a) { return BVec2(isnan(a.x), isnan(a.y)); }
inline BVec2 isinf(const Vec2d &a) { return BVec2(isinf(a.x), isinf(a.y)); }
inline constexpr BVec2 isfinite(const Vec2d &a) { return BVec2(isfinite(a.x), isfinite(a.y)); }
inline Vec2d sin(const Vec2d &a) { return Vec2d(sin(a.x), sin(a.y)); }
inline Vec2d cos(const Vec2d &a) { return Vec2d(cos(a.x), cos(a.y)); }
inline Vec2d tan(const Vec2d &a) { return Vec2d(tan(a.x), tan(a.y)); }
inline constexpr Vec2d radians(const Vec2d &a) { return Vec2d(radians(a.x), radians(a.y)); }
inline constexpr Vec2d degrees(const Vec2d &a) { return Vec2d(degrees(a.x), degrees(a.y)); }
inline Vec2d sqrt(const Vec2d &a) { return Vec2d(sqrt(a.x), sqrt(a.y)); }
inline Vec2d pow(const Vec2d &a, double e) { return Vec2d(pow(a.x, e), pow(a.y, e)); }
inline constexpr Vec2d sign(const Vec2d &a) { return Vec2d(sign(a.x), sign(a.y)); }
inline Vec2d mod(const Vec2d &a, double b) { return Vec2d(mod(a.x, b), mod(a.y, b)); }
inline constexpr const double *value_ptr(const Vec2d &v) { return &v.x; }
inline constexpr double *value_ptr(Vec2d &v) { return &v.x; }

}
