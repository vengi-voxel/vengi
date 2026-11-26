/**
 * @file
 */

#pragma once

#include "math/Mat4Ops.h"
#include "math/Quatf.h"
#include "math/Vec2f.h"

namespace math {

inline constexpr Mat3f mat3_cast(const Quatf &q) {
	const float qxx = q.x * q.x;
	const float qyy = q.y * q.y;
	const float qzz = q.z * q.z;
	const float qxz = q.x * q.z;
	const float qxy = q.x * q.y;
	const float qyz = q.y * q.z;
	const float qwx = q.w * q.x;
	const float qwy = q.w * q.y;
	const float qwz = q.w * q.z;
	Mat3f r(1.0f);
	r[0][0] = 1.0f - 2.0f * (qyy + qzz);
	r[0][1] = 2.0f * (qxy + qwz);
	r[0][2] = 2.0f * (qxz - qwy);
	r[1][0] = 2.0f * (qxy - qwz);
	r[1][1] = 1.0f - 2.0f * (qxx + qzz);
	r[1][2] = 2.0f * (qyz + qwx);
	r[2][0] = 2.0f * (qxz + qwy);
	r[2][1] = 2.0f * (qyz - qwx);
	r[2][2] = 1.0f - 2.0f * (qxx + qyy);
	return r;
}

inline constexpr Mat4f mat4_cast(const Quatf &q) {
	return Mat4f(mat3_cast(q));
}

inline Quatf quat_cast(const Mat3f &m) {
	const float fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
	const float fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
	const float fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
	const float fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];
	int biggestIndex = 0;
	float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
	if (fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
		fourBiggestSquaredMinus1 = fourXSquaredMinus1;
		biggestIndex = 1;
	}
	if (fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
		fourBiggestSquaredMinus1 = fourYSquaredMinus1;
		biggestIndex = 2;
	}
	if (fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
		fourBiggestSquaredMinus1 = fourZSquaredMinus1;
		biggestIndex = 3;
	}
	const float biggestVal = sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
	const float mult = 0.25f / biggestVal;
	switch (biggestIndex) {
	case 0:
		return Quatf(biggestVal, (m[1][2] - m[2][1]) * mult, (m[2][0] - m[0][2]) * mult, (m[0][1] - m[1][0]) * mult);
	case 1:
		return Quatf((m[1][2] - m[2][1]) * mult, biggestVal, (m[0][1] + m[1][0]) * mult, (m[2][0] + m[0][2]) * mult);
	case 2:
		return Quatf((m[2][0] - m[0][2]) * mult, (m[0][1] + m[1][0]) * mult, biggestVal, (m[1][2] + m[2][1]) * mult);
	case 3:
		return Quatf((m[0][1] - m[1][0]) * mult, (m[2][0] + m[0][2]) * mult, (m[1][2] + m[2][1]) * mult, biggestVal);
	default:
		return Quatf(1.0f, 0.0f, 0.0f, 0.0f);
	}
}

inline Quatf quat_cast(const Mat4f &m) {
	return quat_cast(Mat3f(Vec3f(m[0]), Vec3f(m[1]), Vec3f(m[2])));
}

inline Quatf::Quatf(const Mat3f &m) : Quatf(quat_cast(m)) {
}
inline Quatf::Quatf(const Mat4f &m) : Quatf(quat_cast(m)) {
}

inline float roll(const Quatf &q) {
	const float y = 2.0f * (q.x * q.y + q.w * q.z);
	const float x = q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z;
	if (all(epsilonEqual(Vec2f(x, y), Vec2f(0.0f), epsilonf))) {
		return 0.0f;
	}
	return atan(y, x);
}

inline float pitch(const Quatf &q) {
	const float y = 2.0f * (q.y * q.z + q.w * q.x);
	const float x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;
	if (all(epsilonEqual(Vec2f(x, y), Vec2f(0.0f), epsilonf))) {
		return 2.0f * atan(q.x, q.w);
	}
	return atan(y, x);
}

inline float yaw(const Quatf &q) {
	return asin(clamp(-2.0f * (q.x * q.z - q.w * q.y), -1.0f, 1.0f));
}

inline Vec3f eulerAngles(const Quatf &q) {
	return Vec3f(pitch(q), yaw(q), roll(q));
}

inline Quatf::Quatf(const Vec3f &euler) : Quatf(quat_cast(eulerAngleXYZ(euler.x, euler.y, euler.z))) {
}

inline bool decompose(const Mat4f &model, Vec3f &scale, Quatf &orientation, Vec3f &translation, Vec3f &skew, Vec4f &perspective) {
	Mat4f local = model;
	if (epsilonEqual(local[3][3], 0.0f, epsilonf)) {
		return false;
	}
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			local[i][j] /= local[3][3];
		}
	}
	Mat4f perspectiveMatrix = local;
	for (int i = 0; i < 3; ++i) {
		perspectiveMatrix[i][3] = 0.0f;
	}
	perspectiveMatrix[3][3] = 1.0f;
	if (epsilonEqual(determinant(perspectiveMatrix), 0.0f, epsilonf)) {
		return false;
	}
	if (epsilonNotEqual(local[0][3], 0.0f, epsilonf) || epsilonNotEqual(local[1][3], 0.0f, epsilonf) ||
		epsilonNotEqual(local[2][3], 0.0f, epsilonf)) {
		const Vec4f rightHandSide(local[0][3], local[1][3], local[2][3], local[3][3]);
		perspective = transpose(inverse(perspectiveMatrix)) * rightHandSide;
		local[0][3] = local[1][3] = local[2][3] = 0.0f;
		local[3][3] = 1.0f;
	} else {
		perspective = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
	}
	translation = Vec3f(local[3]);
	local[3] = Vec4f(0.0f, 0.0f, 0.0f, local[3].w);
	Vec3f row[3];
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			row[i][j] = local[i][j];
		}
	}
	scale.x = length(row[0]);
	row[0] = row[0] * (1.0f / length(row[0]));
	skew.z = dot(row[0], row[1]);
	row[1] = row[1] - row[0] * skew.z;
	scale.y = length(row[1]);
	row[1] = row[1] * (1.0f / length(row[1]));
	skew.z /= scale.y;
	skew.y = dot(row[0], row[2]);
	row[2] = row[2] - row[0] * skew.y;
	skew.x = dot(row[1], row[2]);
	row[2] = row[2] - row[1] * skew.x;
	scale.z = length(row[2]);
	row[2] = row[2] * (1.0f / length(row[2]));
	skew.y /= scale.z;
	skew.x /= scale.z;
	const Vec3f pdum3 = cross(row[1], row[2]);
	if (dot(row[0], pdum3) < 0.0f) {
		for (int i = 0; i < 3; ++i) {
			scale[i] *= -1.0f;
			row[i] *= -1.0f;
		}
	}
	const float trace = row[0].x + row[1].y + row[2].z;
	if (trace > 0.0f) {
		float root = sqrt(trace + 1.0f);
		orientation.w = 0.5f * root;
		root = 0.5f / root;
		orientation.x = root * (row[1].z - row[2].y);
		orientation.y = root * (row[2].x - row[0].z);
		orientation.z = root * (row[0].y - row[1].x);
	} else {
		int i = 0;
		if (row[1].y > row[0].x) {
			i = 1;
		}
		if (row[2].z > row[i][i]) {
			i = 2;
		}
		const int j = (i + 1) % 3;
		const int k = (i + 2) % 3;
		float root = sqrt(row[i][i] - row[j][j] - row[k][k] + 1.0f);
		orientation[i] = 0.5f * root;
		root = 0.5f / root;
		orientation[j] = root * (row[i][j] + row[j][i]);
		orientation[k] = root * (row[i][k] + row[k][i]);
		orientation.w = root * (row[j][k] - row[k][j]);
	}
	return true;
}

inline Quatf rotation(const Vec3f &orig, const Vec3f &dest) {
	const float cosTheta = dot(orig, dest);
	if (cosTheta >= 1.0f - epsilonf) {
		return Quatf::identity();
	}
	Vec3f rotationAxis;
	if (cosTheta < -1.0f + epsilonf) {
		rotationAxis = cross(Vec3f(0.0f, 0.0f, 1.0f), orig);
		if (length2(rotationAxis) < epsilonf) {
			rotationAxis = cross(Vec3f(1.0f, 0.0f, 0.0f), orig);
		}
		rotationAxis = normalize(rotationAxis);
		return angleAxis(pi, rotationAxis);
	}
	rotationAxis = cross(orig, dest);
	const float s = sqrt((1.0f + cosTheta) * 2.0f);
	const float invs = 1.0f / s;
	return Quatf(s * 0.5f, rotationAxis.x * invs, rotationAxis.y * invs, rotationAxis.z * invs);
}


}
