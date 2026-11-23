/**
 * @file
 */

#pragma once

#include "math/Mat3f.h"
#include "math/Vec4f.h"

namespace math {

struct Mat4x3f;

struct Mat4f {
	Vec4f c[4];
	typedef Vec4f col_type;

	Mat4f() = default;
	static constexpr Mat4f identity() {
		return Mat4f(1.0f);
	}
	explicit constexpr Mat4f(float s) : c{Vec4f(s, 0.0f, 0.0f, 0.0f), Vec4f(0.0f, s, 0.0f, 0.0f), Vec4f(0.0f, 0.0f, s, 0.0f), Vec4f(0.0f, 0.0f, 0.0f, s)} {
	}
	constexpr Mat4f(const Vec4f &c0, const Vec4f &c1, const Vec4f &c2, const Vec4f &c3) : c{c0, c1, c2, c3} {
	}
	constexpr Mat4f(float x0, float y0, float z0, float w0, float x1, float y1, float z1, float w1, float x2, float y2, float z2,
		  float w2, float x3, float y3, float z3, float w3)
		: c{Vec4f(x0, y0, z0, w0), Vec4f(x1, y1, z1, w1), Vec4f(x2, y2, z2, w2), Vec4f(x3, y3, z3, w3)} {
	}
	constexpr Mat4f(const Mat4x3f &m);
	constexpr Mat4f(const Mat3f &m)
		: c{Vec4f(m[0], 0.0f), Vec4f(m[1], 0.0f), Vec4f(m[2], 0.0f), Vec4f(0.0f, 0.0f, 0.0f, 1.0f)} {
	}

	static constexpr int length() { return 4; }

	constexpr Vec4f &operator[](int i) {
		return c[i];
	}
	constexpr const Vec4f &operator[](int i) const {
		return c[i];
	}
};

inline constexpr Mat4f operator-(const Mat4f &m) {
	return Mat4f(-m[0], -m[1], -m[2], -m[3]);
}
inline constexpr Mat4f operator+(const Mat4f &a, const Mat4f &b) {
	return Mat4f(a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]);
}
inline constexpr Mat4f operator-(const Mat4f &a, const Mat4f &b) {
	return Mat4f(a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]);
}
inline constexpr Vec4f operator*(const Mat4f &m, const Vec4f &v) {
	return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
}
inline constexpr Vec4f operator*(const Vec4f &v, const Mat4f &m) {
	return Vec4f(dot(m[0], v), dot(m[1], v), dot(m[2], v), dot(m[3], v));
}

inline constexpr bool operator==(const Mat4f &a, const Mat4f &b) {
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}
inline constexpr bool operator!=(const Mat4f &a, const Mat4f &b) {
	return !(a == b);
}

inline constexpr Mat4f operator*(const Mat4f &a, const Mat4f &b) {
	return Mat4f(a * b[0], a * b[1], a * b[2], a * b[3]);
}

inline constexpr Mat4f operator*(const Mat4f &m, float s) {
	return Mat4f(m[0] * s, m[1] * s, m[2] * s, m[3] * s);
}
inline constexpr Mat4f operator*(float s, const Mat4f &m) {
	return m * s;
}
inline constexpr Mat4f operator/(const Mat4f &m, float s) {
	return Mat4f(m[0] / s, m[1] / s, m[2] / s, m[3] / s);
}

inline constexpr Mat4f &operator*=(Mat4f &a, const Mat4f &b) {
	a = a * b;
	return a;
}

inline constexpr Vec4f column(const Mat4f &m, int i) {
	return m[i];
}

inline constexpr Vec4f row(const Mat4f &m, int i) {
	return Vec4f(m[0][i], m[1][i], m[2][i], m[3][i]);
}

inline constexpr const float *value_ptr(const Mat4f &m) {
	return &m[0].x;
}
inline constexpr float *value_ptr(Mat4f &m) {
	return &m[0].x;
}

inline constexpr Mat4f make_mat4(const float *m) {
	return Mat4f(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
}

inline constexpr Mat4f transpose(const Mat4f &m) {
	return Mat4f(Vec4f(m[0][0], m[1][0], m[2][0], m[3][0]), Vec4f(m[0][1], m[1][1], m[2][1], m[3][1]),
				 Vec4f(m[0][2], m[1][2], m[2][2], m[3][2]), Vec4f(m[0][3], m[1][3], m[2][3], m[3][3]));
}

inline constexpr bool equal(const Mat4f &lhs, const Mat4f &rhs, float eps = 0.0001f) {
	const float *l = value_ptr(lhs);
	const float *r = value_ptr(rhs);
	for (int i = 0; i < 16; ++i) {
		if (!epsilonEqual(l[i], r[i], eps)) {
			return false;
		}
	}
	return true;
}

inline constexpr Mat4f translate(const Mat4f &m, const Vec3f &v) {
	Mat4f result(m);
	result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
	return result;
}

inline constexpr Mat4f translate(const Vec3f &v) {
	return translate(Mat4f(1.0f), v);
}

inline constexpr Mat4f scale(const Mat4f &m, const Vec3f &v) {
	return Mat4f(m[0] * v[0], m[1] * v[1], m[2] * v[2], m[3]);
}

inline constexpr Mat4f scale(const Vec3f &v) {
	return scale(Mat4f(1.0f), v);
}

inline Mat4f rotate(const Mat4f &m, float angle, const Vec3f &v) {
	const float c = cos(angle);
	const float s = sin(angle);
	const Vec3f axis = normalize(v);
	const Vec3f temp = (1.0f - c) * axis;
	Mat4f rot;
	rot[0][0] = c + temp[0] * axis[0];
	rot[0][1] = temp[0] * axis[1] + s * axis[2];
	rot[0][2] = temp[0] * axis[2] - s * axis[1];
	rot[1][0] = temp[1] * axis[0] - s * axis[2];
	rot[1][1] = c + temp[1] * axis[1];
	rot[1][2] = temp[1] * axis[2] + s * axis[0];
	rot[2][0] = temp[2] * axis[0] + s * axis[1];
	rot[2][1] = temp[2] * axis[1] - s * axis[0];
	rot[2][2] = c + temp[2] * axis[2];
	Mat4f result;
	result[0] = m[0] * rot[0][0] + m[1] * rot[0][1] + m[2] * rot[0][2];
	result[1] = m[0] * rot[1][0] + m[1] * rot[1][1] + m[2] * rot[1][2];
	result[2] = m[0] * rot[2][0] + m[1] * rot[2][1] + m[2] * rot[2][2];
	result[3] = m[3];
	return result;
}

inline Mat4f rotate(float angle, const Vec3f &axis) {
	return rotate(Mat4f(1.0f), angle, axis);
}

inline Mat4f rotateX(const Mat4f &m, float angle) {
	return rotate(m, angle, Vec3f(1.0f, 0.0f, 0.0f));
}

inline Mat4f rotateY(const Mat4f &m, float angle) {
	return rotate(m, angle, Vec3f(0.0f, 1.0f, 0.0f));
}

inline Mat4f rotateZ(const Mat4f &m, float angle) {
	return rotate(m, angle, Vec3f(0.0f, 0.0f, 1.0f));
}

// Upper-left 3x3. A negative value means the transform mirrors (1 or 3 negative scale axes).
inline constexpr float det3x3(const Mat4f &m) {
	return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1]) +
		   m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
}

Vec3i transform(const Mat4f &mat, const Vec3i &pos, const Vec3f &pivot);
Vec3f transform(const Mat4f &mat, const Vec3f &pos, const Vec3f &pivot);

inline constexpr Vec3f transform(const Mat4f &mat, const Vec3f &v) {
	const Vec4f &c1 = column(mat, 0);
	const Vec4f &c2 = column(mat, 1);
	const Vec4f &c3 = column(mat, 2);
	return Vec3f(c1.x * v.x + c1.y * v.y + c1.z * v.z + c1.w, c2.x * v.x + c2.y * v.y + c2.z * v.z + c2.w,
				 c3.x * v.x + c3.y * v.y + c3.z * v.z + c3.w);
}

inline constexpr Vec3f rotate(const Mat4f &mat, const Vec3f &v) {
	const Vec3f c1(column(mat, 0));
	const Vec3f c2(column(mat, 1));
	const Vec3f c3(column(mat, 2));
	return Vec3f(dot(c1, v), dot(c2, v), dot(c3, v));
}

inline constexpr Mat3f::Mat3f(const Mat4f &m)
	: c{Vec3f(m[0].x, m[0].y, m[0].z), Vec3f(m[1].x, m[1].y, m[1].z), Vec3f(m[2].x, m[2].y, m[2].z)} {
}

inline constexpr Vec3f project(const Mat4f &m, const Vec3f &p) {
	const Vec4f r = m * Vec4f(p, 1.0f);
	return Vec3f(r) / r.w;
}

}
