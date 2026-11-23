/**
 * @file
 */

#pragma once

#include "math/Vec3f.h"

namespace math {

struct Mat4f;

struct Mat3f {
	Vec3f c[3];

	Mat3f() = default;
	static constexpr Mat3f identity() {
		return Mat3f(1.0f);
	}
	explicit constexpr Mat3f(float s) : c{Vec3f(s, 0.0f, 0.0f), Vec3f(0.0f, s, 0.0f), Vec3f(0.0f, 0.0f, s)} {
	}
	constexpr Mat3f(const Vec3f &c0, const Vec3f &c1, const Vec3f &c2) : c{c0, c1, c2} {
	}
	constexpr Mat3f(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2)
		: c{Vec3f(x0, y0, z0), Vec3f(x1, y1, z1), Vec3f(x2, y2, z2)} {
	}
	constexpr Mat3f(const Mat4f &m);
	static constexpr int length() { return 3; }

	constexpr Vec3f &operator[](int i) {
		return c[i];
	}
	constexpr const Vec3f &operator[](int i) const {
		return c[i];
	}
};

inline constexpr Mat3f operator-(const Mat3f &m) {
	return Mat3f(-m[0], -m[1], -m[2]);
}
inline constexpr Mat3f operator+(const Mat3f &a, const Mat3f &b) {
	return Mat3f(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}
inline constexpr Mat3f operator-(const Mat3f &a, const Mat3f &b) {
	return Mat3f(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}
inline constexpr Mat3f operator*(const Mat3f &m, float s) {
	return Mat3f(m[0] * s, m[1] * s, m[2] * s);
}
inline constexpr Mat3f operator*(float s, const Mat3f &m) {
	return m * s;
}
inline constexpr Mat3f operator/(const Mat3f &m, float s) {
	return Mat3f(m[0] / s, m[1] / s, m[2] / s);
}

inline constexpr Vec3f operator*(const Mat3f &m, const Vec3f &v) {
	return m[0] * v.x + m[1] * v.y + m[2] * v.z;
}

inline constexpr Vec3f operator*(const Vec3f &v, const Mat3f &m) {
	return Vec3f(dot(m[0], v), dot(m[1], v), dot(m[2], v));
}

inline constexpr bool operator==(const Mat3f &a, const Mat3f &b) {
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}
inline constexpr bool operator!=(const Mat3f &a, const Mat3f &b) {
	return !(a == b);
}

inline constexpr Mat3f operator*(const Mat3f &a, const Mat3f &b) {
	return Mat3f(a * b[0], a * b[1], a * b[2]);
}

inline constexpr Mat3f transpose(const Mat3f &m) {
	Mat3f r;
	r[0] = Vec3f(m[0][0], m[1][0], m[2][0]);
	r[1] = Vec3f(m[0][1], m[1][1], m[2][1]);
	r[2] = Vec3f(m[0][2], m[1][2], m[2][2]);
	return r;
}

inline constexpr const float *value_ptr(const Mat3f &m) {
	return &m[0].x;
}
inline constexpr float *value_ptr(Mat3f &m) {
	return &m[0].x;
}

}
