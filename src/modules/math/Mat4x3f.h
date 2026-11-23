/**
 * @file
 */

#pragma once

#include "math/Mat4f.h"

namespace math {

// 4 columns, 3 rows. Column-major.
struct Mat4x3f {
	Vec3f c[4];

	Mat4x3f() = default;
	static constexpr Mat4x3f identity() {
		return Mat4x3f(1.0f);
	}
	explicit constexpr Mat4x3f(float s) : c{Vec3f(s, 0.0f, 0.0f), Vec3f(0.0f, s, 0.0f), Vec3f(0.0f, 0.0f, s), Vec3f(0.0f)} {
	}
	constexpr Mat4x3f(const Vec3f &c0, const Vec3f &c1, const Vec3f &c2, const Vec3f &c3) : c{c0, c1, c2, c3} {
	}
	constexpr Mat4x3f(const Mat4f &m) : c{Vec3f(m[0].x, m[0].y, m[0].z), Vec3f(m[1].x, m[1].y, m[1].z), Vec3f(m[2].x, m[2].y, m[2].z), Vec3f(m[3].x, m[3].y, m[3].z)} {
	}

	constexpr Vec3f &operator[](int i) {
		return c[i];
	}
	constexpr const Vec3f &operator[](int i) const {
		return c[i];
	}
	static constexpr int length() { return 4; }
};

inline constexpr Mat4x3f operator-(const Mat4x3f &m) {
	return Mat4x3f(-m[0], -m[1], -m[2], -m[3]);
}
inline constexpr Mat4x3f operator+(const Mat4x3f &a, const Mat4x3f &b) {
	return Mat4x3f(a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]);
}
inline constexpr Mat4x3f operator-(const Mat4x3f &a, const Mat4x3f &b) {
	return Mat4x3f(a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]);
}
inline constexpr Mat4x3f operator*(const Mat4x3f &m, float s) {
	return Mat4x3f(m[0] * s, m[1] * s, m[2] * s, m[3] * s);
}
inline constexpr Mat4x3f operator*(float s, const Mat4x3f &m) {
	return m * s;
}
inline constexpr Mat4x3f operator/(const Mat4x3f &m, float s) {
	return Mat4x3f(m[0] / s, m[1] / s, m[2] / s, m[3] / s);
}
inline constexpr Vec3f operator*(const Mat4x3f &m, const Vec4f &v) {
	return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
}
inline constexpr bool operator==(const Mat4x3f &a, const Mat4x3f &b) {
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}
inline constexpr bool operator!=(const Mat4x3f &a, const Mat4x3f &b) {
	return !(a == b);
}

inline constexpr Mat4f::Mat4f(const Mat4x3f &m)
	: c{Vec4f(m[0].x, m[0].y, m[0].z, 0.0f), Vec4f(m[1].x, m[1].y, m[1].z, 0.0f), Vec4f(m[2].x, m[2].y, m[2].z, 0.0f),
		Vec4f(m[3].x, m[3].y, m[3].z, 1.0f)} {
}

}
