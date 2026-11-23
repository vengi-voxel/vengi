/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec4.h"
#include "math/Vec3i.h"
#include "math/Vec4i.h"

namespace math {
struct AlignedVec4f;

struct alignas(16) AlignedVec4i {
	union {
		struct { int x, y, z, w; };
		struct { int r, g, b, a; };
	};
	typedef AlignedVec4i col_type;
	typedef int value_type;
	static constexpr int length() { return 4; }

	AlignedVec4i() = default;
	constexpr AlignedVec4i(int s) : x(s), y(s), z(s), w(s) {}
	constexpr AlignedVec4i(int x_, int y_, int z_, int w_) : x(x_), y(y_), z(z_), w(w_) {}

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
	constexpr AlignedVec4i(const Vec4i &v);
	constexpr AlignedVec4i(const Vec3i &v, int w_);
	constexpr AlignedVec4i(const AlignedVec4f &v);
	constexpr AlignedVec4i &operator+=(const AlignedVec4i &o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr AlignedVec4i &operator+=(int s) { x += s; y += s; z += s; w += s; return *this; }
	constexpr AlignedVec4i &operator-=(const AlignedVec4i &o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr AlignedVec4i &operator-=(int s) { x -= s; y -= s; z -= s; w -= s; return *this; }
	constexpr AlignedVec4i &operator*=(const AlignedVec4i &o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr AlignedVec4i &operator*=(int s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr AlignedVec4i &operator/=(const AlignedVec4i &o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
	constexpr AlignedVec4i &operator/=(int s) { x /= s; y /= s; z /= s; w /= s; return *this; }
};

inline constexpr AlignedVec4i operator+(const AlignedVec4i &a, const AlignedVec4i &o) { return AlignedVec4i(a.x + o.x, a.y + o.y, a.z + o.z, a.w + o.w); }
inline constexpr AlignedVec4i operator+(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x + s, a.y + s, a.z + s, a.w + s); }
inline constexpr AlignedVec4i operator+(int s, const AlignedVec4i &a) { return AlignedVec4i(s + a.x, s + a.y, s + a.z, s + a.w); }
inline constexpr AlignedVec4i operator-(const AlignedVec4i &a, const AlignedVec4i &o) { return AlignedVec4i(a.x - o.x, a.y - o.y, a.z - o.z, a.w - o.w); }
inline constexpr AlignedVec4i operator-(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x - s, a.y - s, a.z - s, a.w - s); }
inline constexpr AlignedVec4i operator-(int s, const AlignedVec4i &a) { return AlignedVec4i(s - a.x, s - a.y, s - a.z, s - a.w); }
inline constexpr AlignedVec4i operator*(const AlignedVec4i &a, const AlignedVec4i &o) { return AlignedVec4i(a.x * o.x, a.y * o.y, a.z * o.z, a.w * o.w); }
inline constexpr AlignedVec4i operator*(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x * s, a.y * s, a.z * s, a.w * s); }
inline constexpr AlignedVec4i operator*(int s, const AlignedVec4i &a) { return AlignedVec4i(s * a.x, s * a.y, s * a.z, s * a.w); }
inline constexpr AlignedVec4i operator/(const AlignedVec4i &a, const AlignedVec4i &o) { return AlignedVec4i(a.x / o.x, a.y / o.y, a.z / o.z, a.w / o.w); }
inline constexpr AlignedVec4i operator/(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x / s, a.y / s, a.z / s, a.w / s); }
inline constexpr AlignedVec4i operator/(int s, const AlignedVec4i &a) { return AlignedVec4i(s / a.x, s / a.y, s / a.z, s / a.w); }
inline constexpr AlignedVec4i operator%(const AlignedVec4i &a, const AlignedVec4i &o) { return AlignedVec4i(a.x % o.x, a.y % o.y, a.z % o.z, a.w % o.w); }
inline constexpr AlignedVec4i operator%(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x % s, a.y % s, a.z % s, a.w % s); }
inline constexpr AlignedVec4i operator%(int s, const AlignedVec4i &a) { return AlignedVec4i(s % a.x, s % a.y, s % a.z, s % a.w); }
inline constexpr AlignedVec4i operator-(const AlignedVec4i &a) { return AlignedVec4i(-a.x, -a.y, -a.z, -a.w); }
inline constexpr bool operator==(const AlignedVec4i &a, const AlignedVec4i &o) { return a.x == o.x && a.y == o.y && a.z == o.z && a.w == o.w; }
inline constexpr bool operator!=(const AlignedVec4i &a, const AlignedVec4i &o) { return !(a == o); }
inline constexpr AlignedVec4i operator&(const AlignedVec4i &a, const AlignedVec4i &o) { return AlignedVec4i(a.x & o.x, a.y & o.y, a.z & o.z, a.w & o.w); }
inline constexpr AlignedVec4i operator&(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x & s, a.y & s, a.z & s, a.w & s); }
inline constexpr AlignedVec4i operator|(const AlignedVec4i &a, const AlignedVec4i &o) { return AlignedVec4i(a.x | o.x, a.y | o.y, a.z | o.z, a.w | o.w); }
inline constexpr AlignedVec4i operator|(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x | s, a.y | s, a.z | s, a.w | s); }
inline constexpr AlignedVec4i operator^(const AlignedVec4i &a, const AlignedVec4i &o) { return AlignedVec4i(a.x ^ o.x, a.y ^ o.y, a.z ^ o.z, a.w ^ o.w); }
inline constexpr AlignedVec4i operator^(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x ^ s, a.y ^ s, a.z ^ s, a.w ^ s); }
inline constexpr AlignedVec4i operator~(const AlignedVec4i &a) { return AlignedVec4i(~a.x, ~a.y, ~a.z, ~a.w); }
inline AlignedVec4i operator<<(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x << s, a.y << s, a.z << s, a.w << s); }
inline constexpr AlignedVec4i operator>>(const AlignedVec4i &a, int s) { return AlignedVec4i(a.x >> s, a.y >> s, a.z >> s, a.w >> s); }
inline constexpr int dot(const AlignedVec4i &a, const AlignedVec4i &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline constexpr AlignedVec4i min(const AlignedVec4i &a, const AlignedVec4i &b) { return AlignedVec4i(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w); }
inline constexpr AlignedVec4i max(const AlignedVec4i &a, const AlignedVec4i &b) { return AlignedVec4i(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w); }
inline constexpr AlignedVec4i abs(const AlignedVec4i &a) { return AlignedVec4i(a.x < 0 ? -a.x : a.x, a.y < 0 ? -a.y : a.y, a.z < 0 ? -a.z : a.z, a.w < 0 ? -a.w : a.w); }
inline constexpr BVec4 lessThan(const AlignedVec4i &a, const AlignedVec4i &b) { return BVec4(a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w); }
inline constexpr BVec4 lessThanEqual(const AlignedVec4i &a, const AlignedVec4i &b) { return BVec4(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); }
inline constexpr BVec4 greaterThan(const AlignedVec4i &a, const AlignedVec4i &b) { return BVec4(a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w); }
inline constexpr BVec4 greaterThanEqual(const AlignedVec4i &a, const AlignedVec4i &b) { return BVec4(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }
inline constexpr BVec4 equal(const AlignedVec4i &a, const AlignedVec4i &b) { return BVec4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); }

inline constexpr AlignedVec4i clamp(const AlignedVec4i &v, const AlignedVec4i &lo, const AlignedVec4i &hi) { return AlignedVec4i(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w)); }
inline constexpr AlignedVec4i clamp(const AlignedVec4i &v, int lo, int hi) { return AlignedVec4i(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi)); }
inline constexpr AlignedVec4i min(const AlignedVec4i &a, int s) { return min(a, AlignedVec4i(s)); }
inline constexpr AlignedVec4i max(const AlignedVec4i &a, int s) { return max(a, AlignedVec4i(s)); }
inline constexpr const int *value_ptr(const AlignedVec4i &v) { return &v.x; }
inline constexpr int *value_ptr(AlignedVec4i &v) { return &v.x; }


inline constexpr AlignedVec4i::AlignedVec4i(const Vec4i &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
inline constexpr AlignedVec4i::AlignedVec4i(const Vec3i &v, int w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
inline constexpr Vec4i::Vec4i(const AlignedVec4i &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
inline constexpr Vec4i::Vec4i(const Vec3i &v, int w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
inline constexpr Vec3i::Vec3i(const AlignedVec4i &v) : x(v.x), y(v.y), z(v.z) {}

}
