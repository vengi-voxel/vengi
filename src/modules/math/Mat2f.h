/**
 * @file
 */

#pragma once

#include "math/Vec2f.h"

namespace math {

struct Mat2f {
	Vec2f c[2];

	Mat2f() = default;
	static constexpr Mat2f identity() {
		return Mat2f(1.0f);
	}
	explicit constexpr Mat2f(float s) : c{Vec2f(s, 0.0f), Vec2f(0.0f, s)} {
	}
	constexpr Mat2f(const Vec2f &c0, const Vec2f &c1) : c{c0, c1} {
	}
	constexpr Mat2f(float x0, float y0, float x1, float y1) : c{Vec2f(x0, y0), Vec2f(x1, y1)} {
	}
	static constexpr int length() { return 2; }

	constexpr Vec2f &operator[](int i) {
		return c[i];
	}
	constexpr const Vec2f &operator[](int i) const {
		return c[i];
	}
};

inline constexpr Mat2f operator-(const Mat2f &m) {
	return Mat2f(-m[0], -m[1]);
}
inline constexpr Mat2f operator+(const Mat2f &a, const Mat2f &b) {
	return Mat2f(a[0] + b[0], a[1] + b[1]);
}
inline constexpr Mat2f operator-(const Mat2f &a, const Mat2f &b) {
	return Mat2f(a[0] - b[0], a[1] - b[1]);
}
inline constexpr Mat2f operator*(const Mat2f &m, float s) {
	return Mat2f(m[0] * s, m[1] * s);
}
inline constexpr Mat2f operator*(float s, const Mat2f &m) {
	return m * s;
}
inline constexpr Mat2f operator/(const Mat2f &m, float s) {
	return Mat2f(m[0] / s, m[1] / s);
}
inline constexpr Vec2f operator*(const Mat2f &m, const Vec2f &v) {
	return m[0] * v.x + m[1] * v.y;
}
inline constexpr Vec2f operator*(const Vec2f &v, const Mat2f &m) {
	return Vec2f(dot(m[0], v), dot(m[1], v));
}
inline constexpr Mat2f operator*(const Mat2f &a, const Mat2f &b) {
	return Mat2f(a * b[0], a * b[1]);
}
inline constexpr bool operator==(const Mat2f &a, const Mat2f &b) {
	return a[0] == b[0] && a[1] == b[1];
}
inline constexpr bool operator!=(const Mat2f &a, const Mat2f &b) {
	return !(a == b);
}

inline constexpr float determinant(const Mat2f &m) {
	return m[0][0] * m[1][1] - m[1][0] * m[0][1];
}

}
