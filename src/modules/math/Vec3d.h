/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec3.h"

namespace math {

struct Vec3f;
struct Vec3i;

struct Vec3d {
	union {
		struct { double x, y, z; };
		struct { double r, g, b; };
	};
	typedef Vec3d col_type;
	typedef double value_type;
	static constexpr int length() { return 3; }

	Vec3d() = default;
	constexpr Vec3d(double s) : x(s), y(s), z(s) {}
	constexpr Vec3d(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
	constexpr Vec3d(const Vec3f &v);
	constexpr Vec3d(const Vec3i &v);

	constexpr double &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr double operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		return x;
	}
	constexpr Vec3d &operator+=(const Vec3d &o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr Vec3d &operator+=(double s) { x += s; y += s; z += s; return *this; }
	constexpr Vec3d &operator-=(const Vec3d &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr Vec3d &operator-=(double s) { x -= s; y -= s; z -= s; return *this; }
	constexpr Vec3d &operator*=(const Vec3d &o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	constexpr Vec3d &operator*=(double s) { x *= s; y *= s; z *= s; return *this; }
	constexpr Vec3d &operator/=(const Vec3d &o) { x /= o.x; y /= o.y; z /= o.z; return *this; }
	constexpr Vec3d &operator/=(double s) { x /= s; y /= s; z /= s; return *this; }
};

inline constexpr Vec3d operator+(const Vec3d &a, const Vec3d &o) { return Vec3d(a.x + o.x, a.y + o.y, a.z + o.z); }
inline constexpr Vec3d operator+(const Vec3d &a, double s) { return Vec3d(a.x + s, a.y + s, a.z + s); }
inline constexpr Vec3d operator+(double s, const Vec3d &a) { return Vec3d(s + a.x, s + a.y, s + a.z); }
inline constexpr Vec3d operator-(const Vec3d &a, const Vec3d &o) { return Vec3d(a.x - o.x, a.y - o.y, a.z - o.z); }
inline constexpr Vec3d operator-(const Vec3d &a, double s) { return Vec3d(a.x - s, a.y - s, a.z - s); }
inline constexpr Vec3d operator-(double s, const Vec3d &a) { return Vec3d(s - a.x, s - a.y, s - a.z); }
inline constexpr Vec3d operator*(const Vec3d &a, const Vec3d &o) { return Vec3d(a.x * o.x, a.y * o.y, a.z * o.z); }
inline constexpr Vec3d operator*(const Vec3d &a, double s) { return Vec3d(a.x * s, a.y * s, a.z * s); }
inline constexpr Vec3d operator*(double s, const Vec3d &a) { return Vec3d(s * a.x, s * a.y, s * a.z); }
inline constexpr Vec3d operator/(const Vec3d &a, const Vec3d &o) { return Vec3d(a.x / o.x, a.y / o.y, a.z / o.z); }
inline constexpr Vec3d operator/(const Vec3d &a, double s) { return Vec3d(a.x / s, a.y / s, a.z / s); }
inline constexpr Vec3d operator/(double s, const Vec3d &a) { return Vec3d(s / a.x, s / a.y, s / a.z); }
inline constexpr Vec3d operator-(const Vec3d &a) { return Vec3d(-a.x, -a.y, -a.z); }
inline constexpr bool operator==(const Vec3d &a, const Vec3d &o) { return a.x == o.x && a.y == o.y && a.z == o.z; }
inline constexpr bool operator!=(const Vec3d &a, const Vec3d &o) { return !(a == o); }
inline constexpr double dot(const Vec3d &a, const Vec3d &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr Vec3d min(const Vec3d &a, const Vec3d &b) { return Vec3d(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z); }
inline constexpr Vec3d max(const Vec3d &a, const Vec3d &b) { return Vec3d(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z); }

inline constexpr BVec3 lessThan(const Vec3d &a, const Vec3d &b) { return BVec3(a.x < b.x, a.y < b.y, a.z < b.z); }
inline constexpr BVec3 lessThanEqual(const Vec3d &a, const Vec3d &b) { return BVec3(a.x <= b.x, a.y <= b.y, a.z <= b.z); }
inline constexpr BVec3 greaterThan(const Vec3d &a, const Vec3d &b) { return BVec3(a.x > b.x, a.y > b.y, a.z > b.z); }
inline constexpr BVec3 greaterThanEqual(const Vec3d &a, const Vec3d &b) { return BVec3(a.x >= b.x, a.y >= b.y, a.z >= b.z); }
inline constexpr BVec3 equal(const Vec3d &a, const Vec3d &b) { return BVec3(a.x == b.x, a.y == b.y, a.z == b.z); }
inline constexpr Vec3d cross(const Vec3d &a, const Vec3d &b) { return Vec3d(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
inline constexpr Vec3d abs(const Vec3d &a) { return Vec3d(abs(a.x), abs(a.y), abs(a.z)); }
inline double length(const Vec3d &a) { return sqrt(dot(a, a)); }
inline constexpr double length2(const Vec3d &a) { return dot(a, a); }
inline double distance(const Vec3d &a, const Vec3d &b) { return length(a - b); }
inline constexpr double distance2(const Vec3d &a, const Vec3d &b) { return length2(a - b); }
inline Vec3d normalize(const Vec3d &a) { double len = length(a); return len > 0.0 ? a / len : Vec3d(0.0); }
inline Vec3d floor(const Vec3d &a) { return Vec3d(floor(a.x), floor(a.y), floor(a.z)); }
inline Vec3d ceil(const Vec3d &a) { return Vec3d(ceil(a.x), ceil(a.y), ceil(a.z)); }
inline Vec3d round(const Vec3d &a) { return Vec3d(round(a.x), round(a.y), round(a.z)); }
inline Vec3d fract(const Vec3d &a) { return Vec3d(fract(a.x), fract(a.y), fract(a.z)); }
inline constexpr Vec3d mix(const Vec3d &a, const Vec3d &b, double t) { return Vec3d(mix(a.x, b.x, t), mix(a.y, b.y, t), mix(a.z, b.z, t)); }
inline constexpr Vec3d clamp(const Vec3d &v, double lo, double hi) { return Vec3d(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi)); }
inline constexpr Vec3d clamp(const Vec3d &v, const Vec3d &lo, const Vec3d &hi) { return Vec3d(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z)); }
inline constexpr Vec3d min(const Vec3d &a, double s) { return min(a, Vec3d(s)); }
inline constexpr Vec3d max(const Vec3d &a, double s) { return max(a, Vec3d(s)); }
inline constexpr BVec3 epsilonEqual(const Vec3d &a, const Vec3d &b, double eps) { return BVec3(epsilonEqual(a.x, b.x, eps), epsilonEqual(a.y, b.y, eps), epsilonEqual(a.z, b.z, eps)); }
inline constexpr BVec3 epsilonEqual(const Vec3d &a, const Vec3d &b, const Vec3d &eps) { return BVec3(epsilonEqual(a.x, b.x, eps.x), epsilonEqual(a.y, b.y, eps.y), epsilonEqual(a.z, b.z, eps.z)); }
inline constexpr BVec3 epsilonNotEqual(const Vec3d &a, const Vec3d &b, double eps) { return BVec3(epsilonNotEqual(a.x, b.x, eps), epsilonNotEqual(a.y, b.y, eps), epsilonNotEqual(a.z, b.z, eps)); }
inline constexpr BVec3 isnan(const Vec3d &a) { return BVec3(isnan(a.x), isnan(a.y), isnan(a.z)); }
inline BVec3 isinf(const Vec3d &a) { return BVec3(isinf(a.x), isinf(a.y), isinf(a.z)); }
inline constexpr BVec3 isfinite(const Vec3d &a) { return BVec3(isfinite(a.x), isfinite(a.y), isfinite(a.z)); }
inline Vec3d sin(const Vec3d &a) { return Vec3d(sin(a.x), sin(a.y), sin(a.z)); }
inline Vec3d cos(const Vec3d &a) { return Vec3d(cos(a.x), cos(a.y), cos(a.z)); }
inline Vec3d tan(const Vec3d &a) { return Vec3d(tan(a.x), tan(a.y), tan(a.z)); }
inline constexpr Vec3d radians(const Vec3d &a) { return Vec3d(radians(a.x), radians(a.y), radians(a.z)); }
inline constexpr Vec3d degrees(const Vec3d &a) { return Vec3d(degrees(a.x), degrees(a.y), degrees(a.z)); }
inline Vec3d sqrt(const Vec3d &a) { return Vec3d(sqrt(a.x), sqrt(a.y), sqrt(a.z)); }
inline Vec3d pow(const Vec3d &a, double e) { return Vec3d(pow(a.x, e), pow(a.y, e), pow(a.z, e)); }
inline constexpr Vec3d sign(const Vec3d &a) { return Vec3d(sign(a.x), sign(a.y), sign(a.z)); }
inline Vec3d mod(const Vec3d &a, double b) { return Vec3d(mod(a.x, b), mod(a.y, b), mod(a.z, b)); }
inline constexpr const double *value_ptr(const Vec3d &v) { return &v.x; }
inline constexpr double *value_ptr(Vec3d &v) { return &v.x; }

}
