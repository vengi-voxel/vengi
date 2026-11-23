/**
 * @file
 */

#pragma once

#include "math/BVec4.h"
#include "math/Vec3f.h"

namespace math {

struct Mat3f;
struct Mat4f;

struct Quatf {
	typedef float value_type;
	float x, y, z, w;

	Quatf() = default;
	static constexpr Quatf identity() {
		return Quatf(1.0f, 0.0f, 0.0f, 0.0f);
	}
	constexpr Quatf(float w_, float x_, float y_, float z_) : x(x_), y(y_), z(z_), w(w_) {
	}
	constexpr Quatf(float w_, const Vec3f &v) : x(v.x), y(v.y), z(v.z), w(w_) {
	}
	explicit Quatf(const Vec3f &euler);
	explicit Quatf(const Mat3f &m);
	explicit Quatf(const Mat4f &m);

	static constexpr Quatf wxyz(float w_, float x_, float y_, float z_) {
		return Quatf(w_, x_, y_, z_);
	}

	constexpr float &operator[](int i) {
		return (&x)[i];
	}
	constexpr float operator[](int i) const {
		return (&x)[i];
	}
};

inline constexpr Quatf operator-(const Quatf &q) {
	return Quatf(-q.w, -q.x, -q.y, -q.z);
}

inline constexpr Quatf operator+(const Quatf &a, const Quatf &b) {
	return Quatf(a.w + b.w, a.x + b.x, a.y + b.y, a.z + b.z);
}

inline constexpr Quatf operator-(const Quatf &a, const Quatf &b) {
	return Quatf(a.w - b.w, a.x - b.x, a.y - b.y, a.z - b.z);
}

inline constexpr Quatf operator*(const Quatf &p, const Quatf &q) {
	return Quatf(p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z, p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y,
				 p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z, p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x);
}

inline constexpr Quatf operator*(const Quatf &q, float s) {
	return Quatf(q.w * s, q.x * s, q.y * s, q.z * s);
}

inline constexpr Quatf operator*(float s, const Quatf &q) {
	return q * s;
}

inline constexpr Quatf operator/(const Quatf &q, float s) {
	return Quatf(q.w / s, q.x / s, q.y / s, q.z / s);
}

inline constexpr Vec3f operator*(const Quatf &q, const Vec3f &v) {
	const Vec3f qv(q.x, q.y, q.z);
	const Vec3f uv = cross(qv, v);
	const Vec3f uuv = cross(qv, uv);
	return v + ((uv * q.w) + uuv) * 2.0f;
}

inline constexpr const float *value_ptr(const Quatf &q) {
	return &q.x;
}
inline constexpr float *value_ptr(Quatf &q) {
	return &q.x;
}

inline constexpr bool operator==(const Quatf &a, const Quatf &b) {
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

inline constexpr BVec4 equal(const Quatf &a, const Quatf &b) {
	return BVec4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w);
}

inline constexpr bool operator!=(const Quatf &a, const Quatf &b) {
	return !(a == b);
}

inline constexpr float dot(const Quatf &a, const Quatf &b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline float length(const Quatf &q) {
	return sqrt(dot(q, q));
}

inline Quatf normalize(const Quatf &q) {
	return q / length(q);
}

inline constexpr Quatf conjugate(const Quatf &q) {
	return Quatf(q.w, -q.x, -q.y, -q.z);
}

inline constexpr Quatf inverse(const Quatf &q) {
	return conjugate(q) / dot(q, q);
}

inline constexpr Vec3f operator*(const Vec3f &v, const Quatf &q) {
	return inverse(q) * v;
}

inline Quatf mix(const Quatf &x, const Quatf &y, float a) {
	const float cosTheta = dot(x, y);
	if (cosTheta > 1.0f - epsilonf) {
		return Quatf(mix(x.w, y.w, a), mix(x.x, y.x, a), mix(x.y, y.y, a), mix(x.z, y.z, a));
	}
	const float ang = acos(cosTheta);
	return (sin((1.0f - a) * ang) * x + sin(a * ang) * y) / sin(ang);
}

inline Quatf slerp(const Quatf &x, const Quatf &y, float a) {
	Quatf z = y;
	float cosTheta = dot(x, y);
	if (cosTheta < 0.0f) {
		z = -y;
		cosTheta = -cosTheta;
	}
	if (cosTheta > 1.0f - epsilonf) {
		return Quatf(mix(x.w, z.w, a), mix(x.x, z.x, a), mix(x.y, z.y, a), mix(x.z, z.z, a));
	}
	const float ang = acos(cosTheta);
	return (sin((1.0f - a) * ang) * x + sin(a * ang) * z) / sin(ang);
}

inline Quatf angleAxis(float angle, const Vec3f &v) {
	const float s = sin(angle * 0.5f);
	return Quatf(cos(angle * 0.5f), v.x * s, v.y * s, v.z * s);
}

inline constexpr BVec4 isnan(const Quatf &q) {
	return BVec4(isnan(q.x), isnan(q.y), isnan(q.z), isnan(q.w));
}
inline BVec4 isinf(const Quatf &q) {
	return BVec4(isinf(q.x), isinf(q.y), isinf(q.z), isinf(q.w));
}
inline constexpr BVec4 epsilonEqual(const Quatf &a, const Quatf &b, float eps) {
	return BVec4(epsilonEqual(a.x, b.x, eps), epsilonEqual(a.y, b.y, eps), epsilonEqual(a.z, b.z, eps),
				 epsilonEqual(a.w, b.w, eps));
}

inline Vec3f axis(const Quatf &q) {
	const float tmp1 = 1.0f - q.w * q.w;
	if (tmp1 <= 0.0f) {
		return Vec3f(0.0f, 0.0f, 1.0f);
	}
	const float tmp2 = 1.0f / sqrt(tmp1);
	return Vec3f(q.x * tmp2, q.y * tmp2, q.z * tmp2);
}

inline float angle(const Quatf &q) {
	if (abs(q.w) > cos(0.5f)) {
		const float a = asin(sqrt(q.x * q.x + q.y * q.y + q.z * q.z)) * 2.0f;
		if (q.w < 0.0f) {
			return twoPi - a;
		}
		return a;
	}
	return acos(q.w) * 2.0f;
}

}
