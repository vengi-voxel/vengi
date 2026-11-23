/**
 * @file
 */

#pragma once

#include "math/Vec3d.h"

namespace math {

struct Mat3d {
	Vec3d c[3];

	Mat3d() = default;
	static constexpr Mat3d identity() {
		return Mat3d(1.0);
	}
	explicit constexpr Mat3d(double s) : c{Vec3d(s, 0.0, 0.0), Vec3d(0.0, s, 0.0), Vec3d(0.0, 0.0, s)} {
	}
	constexpr Mat3d(const Vec3d &c0, const Vec3d &c1, const Vec3d &c2) : c{c0, c1, c2} {
	}

	constexpr Vec3d &operator[](int i) {
		return c[i];
	}
	constexpr const Vec3d &operator[](int i) const {
		return c[i];
	}

	constexpr Mat3d &operator+=(const Mat3d &o) {
		c[0] += o.c[0];
		c[1] += o.c[1];
		c[2] += o.c[2];
		return *this;
	}
};

inline constexpr Mat3d operator-(const Mat3d &m) {
	return Mat3d(-m[0], -m[1], -m[2]);
}
inline constexpr Mat3d operator-(const Mat3d &a, const Mat3d &b) {
	return Mat3d(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}
inline constexpr Mat3d operator*(const Mat3d &m, double s) {
	return Mat3d(m[0] * s, m[1] * s, m[2] * s);
}
inline constexpr Mat3d operator*(double s, const Mat3d &m) {
	return m * s;
}
inline constexpr Mat3d operator/(const Mat3d &m, double s) {
	return Mat3d(m[0] / s, m[1] / s, m[2] / s);
}
inline constexpr Vec3d operator*(const Mat3d &m, const Vec3d &v) {
	return m[0] * v.x + m[1] * v.y + m[2] * v.z;
}
inline constexpr Vec3d operator*(const Vec3d &v, const Mat3d &m) {
	return Vec3d(dot(m[0], v), dot(m[1], v), dot(m[2], v));
}
inline constexpr Mat3d operator*(const Mat3d &a, const Mat3d &b) {
	return Mat3d(a * b[0], a * b[1], a * b[2]);
}
inline constexpr bool operator==(const Mat3d &a, const Mat3d &b) {
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}
inline constexpr bool operator!=(const Mat3d &a, const Mat3d &b) {
	return !(a == b);
}

inline constexpr Mat3d operator+(const Mat3d &a, const Mat3d &b) {
	Mat3d r(a);
	r += b;
	return r;
}

inline constexpr Mat3d outerProduct(const Vec3d &c, const Vec3d &r) {
	return Mat3d(c * r.x, c * r.y, c * r.z);
}

inline constexpr Mat3d inverse(const Mat3d &m) {
	const double invDet = 1.0 / (m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
								 m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]) +
								 m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]));
	Mat3d out;
	out[0][0] = +(m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invDet;
	out[1][0] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * invDet;
	out[2][0] = +(m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invDet;
	out[0][1] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * invDet;
	out[1][1] = +(m[0][0] * m[2][2] - m[2][0] * m[0][2]) * invDet;
	out[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * invDet;
	out[0][2] = +(m[0][1] * m[1][2] - m[1][1] * m[0][2]) * invDet;
	out[1][2] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * invDet;
	out[2][2] = +(m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invDet;
	return out;
}

}
