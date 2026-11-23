/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec4.h"

namespace math {

struct Vec4d {
	union {
		struct { double x, y, z, w; };
		struct { double r, g, b, a; };
	};
	typedef Vec4d col_type;
	typedef double value_type;
	static constexpr int length() { return 4; }

	Vec4d() = default;
	constexpr Vec4d(double s) : x(s), y(s), z(s), w(s) {}
	constexpr Vec4d(double x_, double y_, double z_, double w_) : x(x_), y(y_), z(z_), w(w_) {}

	constexpr double &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr double operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr Vec4d &operator+=(const Vec4d &o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr Vec4d &operator+=(double s) { x += s; y += s; z += s; w += s; return *this; }
	constexpr Vec4d &operator-=(const Vec4d &o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr Vec4d &operator-=(double s) { x -= s; y -= s; z -= s; w -= s; return *this; }
	constexpr Vec4d &operator*=(const Vec4d &o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr Vec4d &operator*=(double s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr Vec4d &operator/=(const Vec4d &o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
	constexpr Vec4d &operator/=(double s) { x /= s; y /= s; z /= s; w /= s; return *this; }
};

inline constexpr Vec4d operator+(const Vec4d &a, const Vec4d &o) { return Vec4d(a.x + o.x, a.y + o.y, a.z + o.z, a.w + o.w); }
inline constexpr Vec4d operator+(const Vec4d &a, double s) { return Vec4d(a.x + s, a.y + s, a.z + s, a.w + s); }
inline constexpr Vec4d operator+(double s, const Vec4d &a) { return Vec4d(s + a.x, s + a.y, s + a.z, s + a.w); }
inline constexpr Vec4d operator-(const Vec4d &a, const Vec4d &o) { return Vec4d(a.x - o.x, a.y - o.y, a.z - o.z, a.w - o.w); }
inline constexpr Vec4d operator-(const Vec4d &a, double s) { return Vec4d(a.x - s, a.y - s, a.z - s, a.w - s); }
inline constexpr Vec4d operator-(double s, const Vec4d &a) { return Vec4d(s - a.x, s - a.y, s - a.z, s - a.w); }
inline constexpr Vec4d operator*(const Vec4d &a, const Vec4d &o) { return Vec4d(a.x * o.x, a.y * o.y, a.z * o.z, a.w * o.w); }
inline constexpr Vec4d operator*(const Vec4d &a, double s) { return Vec4d(a.x * s, a.y * s, a.z * s, a.w * s); }
inline constexpr Vec4d operator*(double s, const Vec4d &a) { return Vec4d(s * a.x, s * a.y, s * a.z, s * a.w); }
inline constexpr Vec4d operator/(const Vec4d &a, const Vec4d &o) { return Vec4d(a.x / o.x, a.y / o.y, a.z / o.z, a.w / o.w); }
inline constexpr Vec4d operator/(const Vec4d &a, double s) { return Vec4d(a.x / s, a.y / s, a.z / s, a.w / s); }
inline constexpr Vec4d operator/(double s, const Vec4d &a) { return Vec4d(s / a.x, s / a.y, s / a.z, s / a.w); }
inline constexpr Vec4d operator-(const Vec4d &a) { return Vec4d(-a.x, -a.y, -a.z, -a.w); }
inline constexpr bool operator==(const Vec4d &a, const Vec4d &o) { return a.x == o.x && a.y == o.y && a.z == o.z && a.w == o.w; }
inline constexpr bool operator!=(const Vec4d &a, const Vec4d &o) { return !(a == o); }
inline constexpr double dot(const Vec4d &a, const Vec4d &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline constexpr Vec4d min(const Vec4d &a, const Vec4d &b) { return Vec4d(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w); }
inline constexpr Vec4d max(const Vec4d &a, const Vec4d &b) { return Vec4d(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w); }

inline constexpr BVec4 lessThan(const Vec4d &a, const Vec4d &b) { return BVec4(a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w); }
inline constexpr BVec4 lessThanEqual(const Vec4d &a, const Vec4d &b) { return BVec4(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); }
inline constexpr BVec4 greaterThan(const Vec4d &a, const Vec4d &b) { return BVec4(a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w); }
inline constexpr BVec4 greaterThanEqual(const Vec4d &a, const Vec4d &b) { return BVec4(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }
inline constexpr BVec4 equal(const Vec4d &a, const Vec4d &b) { return BVec4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); }

inline constexpr Vec4d abs(const Vec4d &a) { return Vec4d(abs(a.x), abs(a.y), abs(a.z), abs(a.w)); }
inline double length(const Vec4d &a) { return sqrt(dot(a, a)); }
inline constexpr double length2(const Vec4d &a) { return dot(a, a); }
inline double distance(const Vec4d &a, const Vec4d &b) { return length(a - b); }
inline constexpr double distance2(const Vec4d &a, const Vec4d &b) { return length2(a - b); }
inline Vec4d normalize(const Vec4d &a) { double len = length(a); return len > 0.0 ? a / len : Vec4d(0.0); }
inline Vec4d floor(const Vec4d &a) { return Vec4d(floor(a.x), floor(a.y), floor(a.z), floor(a.w)); }
inline Vec4d ceil(const Vec4d &a) { return Vec4d(ceil(a.x), ceil(a.y), ceil(a.z), ceil(a.w)); }
inline Vec4d round(const Vec4d &a) { return Vec4d(round(a.x), round(a.y), round(a.z), round(a.w)); }
inline Vec4d fract(const Vec4d &a) { return Vec4d(fract(a.x), fract(a.y), fract(a.z), fract(a.w)); }
inline constexpr Vec4d mix(const Vec4d &a, const Vec4d &b, double t) { return Vec4d(mix(a.x, b.x, t), mix(a.y, b.y, t), mix(a.z, b.z, t), mix(a.w, b.w, t)); }
inline constexpr Vec4d clamp(const Vec4d &v, double lo, double hi) { return Vec4d(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi)); }
inline constexpr Vec4d clamp(const Vec4d &v, const Vec4d &lo, const Vec4d &hi) { return Vec4d(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w)); }
inline constexpr Vec4d min(const Vec4d &a, double s) { return min(a, Vec4d(s)); }
inline constexpr Vec4d max(const Vec4d &a, double s) { return max(a, Vec4d(s)); }
inline constexpr BVec4 epsilonEqual(const Vec4d &a, const Vec4d &b, double eps) { return BVec4(epsilonEqual(a.x, b.x, eps), epsilonEqual(a.y, b.y, eps), epsilonEqual(a.z, b.z, eps), epsilonEqual(a.w, b.w, eps)); }
inline constexpr BVec4 epsilonEqual(const Vec4d &a, const Vec4d &b, const Vec4d &eps) { return BVec4(epsilonEqual(a.x, b.x, eps.x), epsilonEqual(a.y, b.y, eps.y), epsilonEqual(a.z, b.z, eps.z), epsilonEqual(a.w, b.w, eps.w)); }
inline constexpr BVec4 epsilonNotEqual(const Vec4d &a, const Vec4d &b, double eps) { return BVec4(epsilonNotEqual(a.x, b.x, eps), epsilonNotEqual(a.y, b.y, eps), epsilonNotEqual(a.z, b.z, eps), epsilonNotEqual(a.w, b.w, eps)); }
inline constexpr BVec4 isnan(const Vec4d &a) { return BVec4(isnan(a.x), isnan(a.y), isnan(a.z), isnan(a.w)); }
inline BVec4 isinf(const Vec4d &a) { return BVec4(isinf(a.x), isinf(a.y), isinf(a.z), isinf(a.w)); }
inline constexpr BVec4 isfinite(const Vec4d &a) { return BVec4(isfinite(a.x), isfinite(a.y), isfinite(a.z), isfinite(a.w)); }
inline Vec4d sin(const Vec4d &a) { return Vec4d(sin(a.x), sin(a.y), sin(a.z), sin(a.w)); }
inline Vec4d cos(const Vec4d &a) { return Vec4d(cos(a.x), cos(a.y), cos(a.z), cos(a.w)); }
inline Vec4d tan(const Vec4d &a) { return Vec4d(tan(a.x), tan(a.y), tan(a.z), tan(a.w)); }
inline constexpr Vec4d radians(const Vec4d &a) { return Vec4d(radians(a.x), radians(a.y), radians(a.z), radians(a.w)); }
inline constexpr Vec4d degrees(const Vec4d &a) { return Vec4d(degrees(a.x), degrees(a.y), degrees(a.z), degrees(a.w)); }
inline Vec4d sqrt(const Vec4d &a) { return Vec4d(sqrt(a.x), sqrt(a.y), sqrt(a.z), sqrt(a.w)); }
inline Vec4d pow(const Vec4d &a, double e) { return Vec4d(pow(a.x, e), pow(a.y, e), pow(a.z, e), pow(a.w, e)); }
inline constexpr Vec4d sign(const Vec4d &a) { return Vec4d(sign(a.x), sign(a.y), sign(a.z), sign(a.w)); }
inline Vec4d mod(const Vec4d &a, double b) { return Vec4d(mod(a.x, b), mod(a.y, b), mod(a.z, b), mod(a.w, b)); }
inline constexpr const double *value_ptr(const Vec4d &v) { return &v.x; }
inline constexpr double *value_ptr(Vec4d &v) { return &v.x; }

}
