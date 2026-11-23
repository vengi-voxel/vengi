/**
 * @file
 */

#pragma once

#include "math/Scalar.h"
#include "math/BVec4.h"

namespace math {

struct U8Vec4 {
	union {
		struct { uint8_t x, y, z, w; };
		struct { uint8_t r, g, b, a; };
	};
	typedef U8Vec4 col_type;
	typedef uint8_t value_type;
	static constexpr int length() { return 4; }

	U8Vec4() = default;
	constexpr U8Vec4(uint8_t s) : x(s), y(s), z(s), w(s) {}
	constexpr U8Vec4(uint8_t x_, uint8_t y_, uint8_t z_, uint8_t w_) : x(x_), y(y_), z(z_), w(w_) {}

	constexpr uint8_t &operator[](int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr uint8_t operator[](int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		if (i == 2) return z;
		if (i == 3) return w;
		return x;
	}
	constexpr U8Vec4 &operator+=(const U8Vec4 &o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	constexpr U8Vec4 &operator+=(uint8_t s) { x += s; y += s; z += s; w += s; return *this; }
	constexpr U8Vec4 &operator-=(const U8Vec4 &o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	constexpr U8Vec4 &operator-=(uint8_t s) { x -= s; y -= s; z -= s; w -= s; return *this; }
	constexpr U8Vec4 &operator*=(const U8Vec4 &o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
	constexpr U8Vec4 &operator*=(uint8_t s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	constexpr U8Vec4 &operator/=(const U8Vec4 &o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
	constexpr U8Vec4 &operator/=(uint8_t s) { x /= s; y /= s; z /= s; w /= s; return *this; }
};

inline constexpr U8Vec4 operator+(const U8Vec4 &a, const U8Vec4 &o) { return U8Vec4(a.x + o.x, a.y + o.y, a.z + o.z, a.w + o.w); }
inline constexpr U8Vec4 operator+(const U8Vec4 &a, uint8_t s) { return U8Vec4(a.x + s, a.y + s, a.z + s, a.w + s); }
inline constexpr U8Vec4 operator+(uint8_t s, const U8Vec4 &a) { return U8Vec4(s + a.x, s + a.y, s + a.z, s + a.w); }
inline constexpr U8Vec4 operator-(const U8Vec4 &a, const U8Vec4 &o) { return U8Vec4(a.x - o.x, a.y - o.y, a.z - o.z, a.w - o.w); }
inline constexpr U8Vec4 operator-(const U8Vec4 &a, uint8_t s) { return U8Vec4(a.x - s, a.y - s, a.z - s, a.w - s); }
inline constexpr U8Vec4 operator-(uint8_t s, const U8Vec4 &a) { return U8Vec4(s - a.x, s - a.y, s - a.z, s - a.w); }
inline constexpr U8Vec4 operator*(const U8Vec4 &a, const U8Vec4 &o) { return U8Vec4(a.x * o.x, a.y * o.y, a.z * o.z, a.w * o.w); }
inline constexpr U8Vec4 operator*(const U8Vec4 &a, uint8_t s) { return U8Vec4(a.x * s, a.y * s, a.z * s, a.w * s); }
inline constexpr U8Vec4 operator*(uint8_t s, const U8Vec4 &a) { return U8Vec4(s * a.x, s * a.y, s * a.z, s * a.w); }
inline constexpr U8Vec4 operator/(const U8Vec4 &a, const U8Vec4 &o) { return U8Vec4(a.x / o.x, a.y / o.y, a.z / o.z, a.w / o.w); }
inline constexpr U8Vec4 operator/(const U8Vec4 &a, uint8_t s) { return U8Vec4(a.x / s, a.y / s, a.z / s, a.w / s); }
inline constexpr U8Vec4 operator/(uint8_t s, const U8Vec4 &a) { return U8Vec4(s / a.x, s / a.y, s / a.z, s / a.w); }
inline constexpr U8Vec4 operator%(const U8Vec4 &a, const U8Vec4 &o) { return U8Vec4(a.x % o.x, a.y % o.y, a.z % o.z, a.w % o.w); }
inline constexpr U8Vec4 operator%(const U8Vec4 &a, uint8_t s) { return U8Vec4(a.x % s, a.y % s, a.z % s, a.w % s); }
inline constexpr U8Vec4 operator%(uint8_t s, const U8Vec4 &a) { return U8Vec4(s % a.x, s % a.y, s % a.z, s % a.w); }
inline constexpr U8Vec4 operator-(const U8Vec4 &a) { return U8Vec4(-a.x, -a.y, -a.z, -a.w); }
inline constexpr bool operator==(const U8Vec4 &a, const U8Vec4 &o) { return a.x == o.x && a.y == o.y && a.z == o.z && a.w == o.w; }
inline constexpr bool operator!=(const U8Vec4 &a, const U8Vec4 &o) { return !(a == o); }
inline constexpr uint8_t dot(const U8Vec4 &a, const U8Vec4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline constexpr U8Vec4 min(const U8Vec4 &a, const U8Vec4 &b) { return U8Vec4(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w); }
inline constexpr U8Vec4 max(const U8Vec4 &a, const U8Vec4 &b) { return U8Vec4(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w); }
inline constexpr U8Vec4 abs(const U8Vec4 &a) { return a; }
inline constexpr BVec4 lessThan(const U8Vec4 &a, const U8Vec4 &b) { return BVec4(a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w); }
inline constexpr BVec4 lessThanEqual(const U8Vec4 &a, const U8Vec4 &b) { return BVec4(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); }
inline constexpr BVec4 greaterThan(const U8Vec4 &a, const U8Vec4 &b) { return BVec4(a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w); }
inline constexpr BVec4 greaterThanEqual(const U8Vec4 &a, const U8Vec4 &b) { return BVec4(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }
inline constexpr BVec4 equal(const U8Vec4 &a, const U8Vec4 &b) { return BVec4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); }

inline constexpr U8Vec4 clamp(const U8Vec4 &v, const U8Vec4 &lo, const U8Vec4 &hi) { return U8Vec4(clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w)); }
inline constexpr U8Vec4 clamp(const U8Vec4 &v, uint8_t lo, uint8_t hi) { return U8Vec4(clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi)); }
inline constexpr U8Vec4 min(const U8Vec4 &a, uint8_t s) { return min(a, U8Vec4(s)); }
inline constexpr U8Vec4 max(const U8Vec4 &a, uint8_t s) { return max(a, U8Vec4(s)); }
inline constexpr U8Vec4 mix(const U8Vec4 &a, const U8Vec4 &b, float t) {
	const float s = 1.0f - t;
	return U8Vec4((uint8_t)((float)a.x * s + (float)b.x * t), (uint8_t)((float)a.y * s + (float)b.y * t),
				  (uint8_t)((float)a.z * s + (float)b.z * t), (uint8_t)((float)a.w * s + (float)b.w * t));
}
inline constexpr const uint8_t *value_ptr(const U8Vec4 &v) { return &v.x; }
inline constexpr uint8_t *value_ptr(U8Vec4 &v) { return &v.x; }

}
