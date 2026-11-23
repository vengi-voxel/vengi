/**
 * @file
 */

#pragma once

#include "math/Mat3f.h"
#include "math/Vec4f.h"

namespace math {

// 3 columns, 4 rows. Column-major.
struct Mat3x4f {
	Vec4f c[3];

	Mat3x4f() = default;
	static constexpr Mat3x4f identity() {
		return Mat3x4f(Vec4f(1.0f, 0.0f, 0.0f, 0.0f), Vec4f(0.0f, 1.0f, 0.0f, 0.0f), Vec4f(0.0f, 0.0f, 1.0f, 0.0f));
	}
	constexpr Mat3x4f(const Vec4f &c0, const Vec4f &c1, const Vec4f &c2) : c{c0, c1, c2} {
	}
	static constexpr int length() { return 3; }

	constexpr Vec4f &operator[](int i) {
		return c[i];
	}
	constexpr const Vec4f &operator[](int i) const {
		return c[i];
	}
};

inline constexpr Mat3x4f operator-(const Mat3x4f &m) {
	return Mat3x4f(-m[0], -m[1], -m[2]);
}
inline constexpr Mat3x4f operator+(const Mat3x4f &a, const Mat3x4f &b) {
	return Mat3x4f(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}
inline constexpr Mat3x4f operator-(const Mat3x4f &a, const Mat3x4f &b) {
	return Mat3x4f(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}
inline constexpr Mat3x4f operator*(const Mat3x4f &m, float s) {
	return Mat3x4f(m[0] * s, m[1] * s, m[2] * s);
}
inline constexpr Mat3x4f operator*(float s, const Mat3x4f &m) {
	return m * s;
}
inline constexpr Mat3x4f operator/(const Mat3x4f &m, float s) {
	return Mat3x4f(m[0] / s, m[1] / s, m[2] / s);
}
inline constexpr Vec4f operator*(const Mat3x4f &m, const Vec3f &v) {
	return m[0] * v.x + m[1] * v.y + m[2] * v.z;
}
inline constexpr bool operator==(const Mat3x4f &a, const Mat3x4f &b) {
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}
inline constexpr bool operator!=(const Mat3x4f &a, const Mat3x4f &b) {
	return !(a == b);
}

inline constexpr Mat3x4f operator*(const Mat3x4f &lhs, const Mat3x4f &o) {
	Vec4f c1 = o[0] * lhs[0].x + o[1] * lhs[0].y + o[2] * lhs[0].z;
	c1.w += lhs[0].w;
	Vec4f c2 = o[0] * lhs[1].x + o[1] * lhs[1].y + o[2] * lhs[1].z;
	c2.w += lhs[1].w;
	Vec4f c3 = o[0] * lhs[2].x + o[1] * lhs[2].y + o[2] * lhs[2].z;
	c3.w += lhs[2].w;
	return Mat3x4f(c1, c2, c3);
}

inline constexpr Mat3x4f invert(const Mat3x4f &o) {
	Mat3f invrot(Vec3f(o[0].x, o[1].x, o[2].x), Vec3f(o[0].y, o[1].y, o[2].y), Vec3f(o[0].z, o[1].z, o[2].z));
	invrot[0] /= length2(invrot[0]);
	invrot[1] /= length2(invrot[1]);
	invrot[2] /= length2(invrot[2]);
	const Vec3f trans(o[0].w, o[1].w, o[2].w);
	const Vec4f a(invrot[0], -dot(invrot[0], trans));
	const Vec4f b(invrot[1], -dot(invrot[1], trans));
	const Vec4f c(invrot[2], -dot(invrot[2], trans));
	return Mat3x4f(a, b, c);
}

}
