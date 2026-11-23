/**
 * @file
 */

#pragma once

#include "math/Mat4f.h"

namespace math {

inline constexpr Mat4f inverse(const Mat4f &m) {
	const float coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
	const float coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
	const float coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];
	const float coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
	const float coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
	const float coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];
	const float coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
	const float coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
	const float coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];
	const float coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
	const float coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
	const float coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];
	const float coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
	const float coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
	const float coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];
	const float coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
	const float coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
	const float coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

	const Vec4f fac0(coef00, coef00, coef02, coef03);
	const Vec4f fac1(coef04, coef04, coef06, coef07);
	const Vec4f fac2(coef08, coef08, coef10, coef11);
	const Vec4f fac3(coef12, coef12, coef14, coef15);
	const Vec4f fac4(coef16, coef16, coef18, coef19);
	const Vec4f fac5(coef20, coef20, coef22, coef23);
	const Vec4f vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
	const Vec4f vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
	const Vec4f vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
	const Vec4f vec3(m[1][3], m[0][3], m[0][3], m[0][3]);
	const Vec4f inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
	const Vec4f inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
	const Vec4f inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
	const Vec4f inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);
	const Vec4f signA(1.0f, -1.0f, 1.0f, -1.0f);
	const Vec4f signB(-1.0f, 1.0f, -1.0f, 1.0f);
	Mat4f inverseMat(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);
	const Vec4f row0(inverseMat[0][0], inverseMat[1][0], inverseMat[2][0], inverseMat[3][0]);
	const Vec4f dot0(m[0] * row0);
	const float invDet = 1.0f / ((dot0.x + dot0.y) + (dot0.z + dot0.w));
	return inverseMat * invDet;
}

inline constexpr float determinant(const Mat4f &m) {
	const float coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
	const float coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
	const float coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
	const float coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
	const float coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
	const float coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
	const float inv0 = m[1][1] * coef00 - m[1][2] * coef04 + m[1][3] * coef08;
	const float inv1 = m[1][0] * coef00 - m[1][2] * coef12 + m[1][3] * coef16;
	const float inv2 = m[1][0] * coef04 - m[1][1] * coef12 + m[1][3] * coef20;
	const float inv3 = m[1][0] * coef08 - m[1][1] * coef16 + m[1][2] * coef20;
	return m[0][0] * inv0 - m[0][1] * inv1 + m[0][2] * inv2 - m[0][3] * inv3;
}

inline constexpr Mat3f inverse(const Mat3f &m) {
	const float invDet = 1.0f / (m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
								 m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]) +
								 m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]));
	Mat3f r;
	r[0][0] = +(m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invDet;
	r[1][0] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * invDet;
	r[2][0] = +(m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invDet;
	r[0][1] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * invDet;
	r[1][1] = +(m[0][0] * m[2][2] - m[2][0] * m[0][2]) * invDet;
	r[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * invDet;
	r[0][2] = +(m[0][1] * m[1][2] - m[1][1] * m[0][2]) * invDet;
	r[1][2] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * invDet;
	r[2][2] = +(m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invDet;
	return r;
}

inline Mat4f eulerAngleX(float angleX) {
	const float cx = cos(angleX);
	const float sx = sin(angleX);
	Mat4f r(1.0f);
	r[1][1] = cx;
	r[1][2] = sx;
	r[2][1] = -sx;
	r[2][2] = cx;
	return r;
}

inline Mat4f eulerAngleY(float angleY) {
	const float cy = cos(angleY);
	const float sy = sin(angleY);
	Mat4f r(1.0f);
	r[0][0] = cy;
	r[0][2] = -sy;
	r[2][0] = sy;
	r[2][2] = cy;
	return r;
}

inline Mat4f eulerAngleZ(float angleZ) {
	const float cz = cos(angleZ);
	const float sz = sin(angleZ);
	Mat4f r(1.0f);
	r[0][0] = cz;
	r[0][1] = sz;
	r[1][0] = -sz;
	r[1][1] = cz;
	return r;
}

inline Mat4f eulerAngleXYZ(float x, float y, float z) {
	const float c1 = cos(-x);
	const float c2 = cos(-y);
	const float c3 = cos(-z);
	const float s1 = sin(-x);
	const float s2 = sin(-y);
	const float s3 = sin(-z);
	Mat4f r(1.0f);
	r[0][0] = c2 * c3;
	r[0][1] = -c1 * s3 + s1 * s2 * c3;
	r[0][2] = s1 * s3 + c1 * s2 * c3;
	r[0][3] = 0.0f;
	r[1][0] = c2 * s3;
	r[1][1] = c1 * c3 + s1 * s2 * s3;
	r[1][2] = -s1 * c3 + c1 * s2 * s3;
	r[1][3] = 0.0f;
	r[2][0] = -s2;
	r[2][1] = s1 * c2;
	r[2][2] = c1 * c2;
	r[2][3] = 0.0f;
	return r;
}

inline Mat4f eulerAngleYXZ(float yaw, float pitch, float roll) {
	const float ch = cos(yaw);
	const float sh = sin(yaw);
	const float cp = cos(pitch);
	const float sp = sin(pitch);
	const float cb = cos(roll);
	const float sb = sin(roll);
	Mat4f r(1.0f);
	r[0][0] = ch * cb + sh * sp * sb;
	r[0][1] = sb * cp;
	r[0][2] = -sh * cb + ch * sp * sb;
	r[1][0] = -ch * sb + sh * sp * cb;
	r[1][1] = cb * cp;
	r[1][2] = sb * sh + ch * sp * cb;
	r[2][0] = sh * cp;
	r[2][1] = -sp;
	r[2][2] = ch * cp;
	return r;
}

inline Mat4f eulerAngleXZY(float t1, float t2, float t3) {
	const float c1 = cos(t1);
	const float s1 = sin(t1);
	const float c2 = cos(t2);
	const float s2 = sin(t2);
	const float c3 = cos(t3);
	const float s3 = sin(t3);
	Mat4f r(1.0f);
	r[0][0] = c2 * c3;
	r[0][1] = s1 * s3 + c1 * c3 * s2;
	r[0][2] = c3 * s1 * s2 - c1 * s3;
	r[1][0] = -s2;
	r[1][1] = c1 * c2;
	r[1][2] = c2 * s1;
	r[2][0] = c2 * s3;
	r[2][1] = c1 * s2 * s3 - c3 * s1;
	r[2][2] = c1 * c3 + s1 * s2 * s3;
	return r;
}

inline Mat4f eulerAngleYZX(float t1, float t2, float t3) {
	const float c1 = cos(t1);
	const float s1 = sin(t1);
	const float c2 = cos(t2);
	const float s2 = sin(t2);
	const float c3 = cos(t3);
	const float s3 = sin(t3);
	Mat4f r(1.0f);
	r[0][0] = c1 * c2;
	r[0][1] = s2;
	r[0][2] = -c2 * s1;
	r[1][0] = s1 * s3 - c1 * c3 * s2;
	r[1][1] = c2 * c3;
	r[1][2] = c1 * s3 + c3 * s1 * s2;
	r[2][0] = c3 * s1 + c1 * s2 * s3;
	r[2][1] = -c2 * s3;
	r[2][2] = c1 * c3 - s1 * s2 * s3;
	return r;
}

inline Mat4f eulerAngleZYX(float t1, float t2, float t3) {
	const float c1 = cos(t1);
	const float s1 = sin(t1);
	const float c2 = cos(t2);
	const float s2 = sin(t2);
	const float c3 = cos(t3);
	const float s3 = sin(t3);
	Mat4f r(1.0f);
	r[0][0] = c1 * c2;
	r[0][1] = c2 * s1;
	r[0][2] = -s2;
	r[1][0] = c1 * s2 * s3 - c3 * s1;
	r[1][1] = c1 * c3 + s1 * s2 * s3;
	r[1][2] = c2 * s3;
	r[2][0] = s1 * s3 + c1 * c3 * s2;
	r[2][1] = c3 * s1 * s2 - c1 * s3;
	r[2][2] = c2 * c3;
	return r;
}

inline Mat4f eulerAngleZXY(float t1, float t2, float t3) {
	const float c1 = cos(t1);
	const float s1 = sin(t1);
	const float c2 = cos(t2);
	const float s2 = sin(t2);
	const float c3 = cos(t3);
	const float s3 = sin(t3);
	Mat4f r(1.0f);
	r[0][0] = c1 * c3 - s1 * s2 * s3;
	r[0][1] = c3 * s1 + c1 * s2 * s3;
	r[0][2] = -c2 * s3;
	r[1][0] = -c2 * s1;
	r[1][1] = c1 * c2;
	r[1][2] = s2;
	r[2][0] = c1 * s3 + c3 * s1 * s2;
	r[2][1] = s1 * s3 - c1 * c3 * s2;
	r[2][2] = c2 * c3;
	return r;
}

inline constexpr Mat4f orthoRH_NO(float left, float right, float bottom, float top, float zNear, float zFar) {
	Mat4f r(1.0f);
	r[0][0] = 2.0f / (right - left);
	r[1][1] = 2.0f / (top - bottom);
	r[2][2] = -2.0f / (zFar - zNear);
	r[3][0] = -(right + left) / (right - left);
	r[3][1] = -(top + bottom) / (top - bottom);
	r[3][2] = -(zFar + zNear) / (zFar - zNear);
	return r;
}

inline constexpr Mat4f orthoRH_ZO(float left, float right, float bottom, float top, float zNear, float zFar) {
	Mat4f r(1.0f);
	r[0][0] = 2.0f / (right - left);
	r[1][1] = 2.0f / (top - bottom);
	r[2][2] = -1.0f / (zFar - zNear);
	r[3][0] = -(right + left) / (right - left);
	r[3][1] = -(top + bottom) / (top - bottom);
	r[3][2] = -zNear / (zFar - zNear);
	return r;
}

inline constexpr Mat4f ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
	return orthoRH_NO(left, right, bottom, top, zNear, zFar);
}

inline constexpr Mat4f ortho(float left, float right, float bottom, float top) {
	Mat4f r(1.0f);
	r[0][0] = 2.0f / (right - left);
	r[1][1] = 2.0f / (top - bottom);
	r[2][2] = -1.0f;
	r[3][0] = -(right + left) / (right - left);
	r[3][1] = -(top + bottom) / (top - bottom);
	return r;
}

inline constexpr Mat4f orthoZO(float left, float right, float bottom, float top, float zNear, float zFar) {
	return orthoRH_ZO(left, right, bottom, top, zNear, zFar);
}

inline constexpr Mat4f orthoRH(float left, float right, float bottom, float top, float zNear, float zFar) {
	return orthoRH_NO(left, right, bottom, top, zNear, zFar);
}

inline Mat4f perspective(float fovy, float aspect, float zNear, float zFar) {
	const float tanHalf = tan(fovy * 0.5f);
	Mat4f r(0.0f);
	r[0][0] = 1.0f / (aspect * tanHalf);
	r[1][1] = 1.0f / tanHalf;
	r[2][2] = -(zFar + zNear) / (zFar - zNear);
	r[2][3] = -1.0f;
	r[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
	return r;
}

inline Mat4f perspectiveFovRH_ZO(float fov, float width, float height, float zNear, float zFar) {
	const float h = cos(0.5f * fov) / sin(0.5f * fov);
	const float w = h * height / width;
	Mat4f r(0.0f);
	r[0][0] = w;
	r[1][1] = h;
	r[2][2] = zFar / (zNear - zFar);
	r[2][3] = -1.0f;
	r[3][2] = -(zFar * zNear) / (zFar - zNear);
	return r;
}

inline Mat4f perspectiveFovRH_NO(float fov, float width, float height, float zNear, float zFar) {
	const float h = cos(0.5f * fov) / sin(0.5f * fov);
	const float w = h * height / width;
	Mat4f r(0.0f);
	r[0][0] = w;
	r[1][1] = h;
	r[2][2] = -(zFar + zNear) / (zFar - zNear);
	r[2][3] = -1.0f;
	r[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
	return r;
}

inline Mat4f perspectiveFovRH(float fov, float width, float height, float zNear, float zFar) {
	return perspectiveFovRH_NO(fov, width, height, zNear, zFar);
}

inline Mat4f lookAt(const Vec3f &eye, const Vec3f &center, const Vec3f &up) {
	const Vec3f f = normalize(center - eye);
	const Vec3f s = normalize(cross(f, up));
	const Vec3f u = cross(s, f);
	Mat4f r(1.0f);
	r[0][0] = s.x;
	r[1][0] = s.y;
	r[2][0] = s.z;
	r[0][1] = u.x;
	r[1][1] = u.y;
	r[2][1] = u.z;
	r[0][2] = -f.x;
	r[1][2] = -f.y;
	r[2][2] = -f.z;
	r[3][0] = -dot(s, eye);
	r[3][1] = -dot(u, eye);
	r[3][2] = dot(f, eye);
	return r;
}

inline constexpr Vec3f unProject(const Vec3f &win, const Mat4f &model, const Mat4f &proj, const Vec4f &viewport, bool zeroToOne) {
	const Mat4f inverseMat = inverse(proj * model);
	Vec4f tmp((win.x - viewport[0]) / viewport[2], (win.y - viewport[1]) / viewport[3], win.z, 1.0f);
	tmp.x = tmp.x * 2.0f - 1.0f;
	tmp.y = tmp.y * 2.0f - 1.0f;
	if (!zeroToOne) {
		tmp.z = tmp.z * 2.0f - 1.0f;
	}
	const Vec4f obj = inverseMat * tmp;
	const float invW = 1.0f / obj.w;
	return Vec3f(obj.x * invW, obj.y * invW, obj.z * invW);
}

inline constexpr Vec3f unProjectZO(const Vec3f &win, const Mat4f &model, const Mat4f &proj, const Vec4f &viewport) {
	return unProject(win, model, proj, viewport, true);
}

inline constexpr Vec3f unProject(const Vec3f &win, const Mat4f &model, const Mat4f &proj, const Vec4f &viewport) {
	return unProject(win, model, proj, viewport, false);
}


}
