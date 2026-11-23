/**
 * @file
 * @brief Signed distance functions based on https://iquilezles.org/articles/distfunctions/
 */

#include "math/SDF.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"

namespace math {
namespace sdf {

float goxSphere(const math::Vec3f &p, const math::Vec3f &s) {
	const float d = math::length(p);
	if (d == 0.0f) {
		return math::max(s.x, math::max(s.y, s.z));
	}
	return d / math::length(p / s) - d;
}

float goxCube(const math::Vec3f &p, const math::Vec3f &s) {
	if (math::any(math::lessThan(p, -s)) || math::any(math::greaterThanEqual(p, s))) {
		return -1.0f;
	}
	const math::Vec3f ap = math::abs(p);
	const math::Vec3f ratio = ap / s;
	int axis = 0;
	if (ratio.y > ratio[axis]) {
		axis = 1;
	}
	if (ratio.z > ratio[axis]) {
		axis = 2;
	}
	return s[axis] - ap[axis];
}

float goxCylinder(const math::Vec3f &p, const math::Vec3f &s) {
	const math::Vec2f pxy(p.x, p.y);
	const math::Vec2f sxy(s.x, s.y);
	const float d = math::length(pxy);
	const float rz = s.z - math::abs(p.z);
	if (d == 0.0f) {
		return math::min(rz, math::max(s.x, math::max(s.y, s.z)));
	}
	return math::min(rz, d / math::length(pxy / sxy) - d);
}

float sphere(const math::Vec3f &p, float r) {
	return math::length(p) - r;
}

float box(const math::Vec3f &p, const math::Vec3f &b) {
	const math::Vec3f q = math::abs(p) - b;
	return math::length(math::max(q, 0.0f)) + math::min(math::max(q.x, math::max(q.y, q.z)), 0.0f);
}

float roundBox(const math::Vec3f &p, const math::Vec3f &b, float r) {
	const math::Vec3f q = math::abs(p) - b + r;
	return math::length(math::max(q, 0.0f)) + math::min(math::max(q.x, math::max(q.y, q.z)), 0.0f) - r;
}

float boxFrame(const math::Vec3f &p, const math::Vec3f &b, float e) {
	math::Vec3f ap = math::abs(p) - b;
	const math::Vec3f q = math::abs(ap + e) - e;
	return math::min(math::min(
		math::length(math::max(math::Vec3f(ap.x, q.y, q.z), 0.0f)) + math::min(math::max(ap.x, math::max(q.y, q.z)), 0.0f),
		math::length(math::max(math::Vec3f(q.x, ap.y, q.z), 0.0f)) + math::min(math::max(q.x, math::max(ap.y, q.z)), 0.0f)),
		math::length(math::max(math::Vec3f(q.x, q.y, ap.z), 0.0f)) + math::min(math::max(q.x, math::max(q.y, ap.z)), 0.0f));
}

float torus(const math::Vec3f &p, float majorRadius, float minorRadius) {
	const math::Vec2f q(math::length(math::Vec2f(p.x, p.z)) - majorRadius, p.y);
	return math::length(q) - minorRadius;
}

float capsule(const math::Vec3f &p, const math::Vec3f &a, const math::Vec3f &b, float r) {
	const math::Vec3f pa = p - a;
	const math::Vec3f ba = b - a;
	const float h = math::clamp(math::dot(pa, ba) / math::dot(ba, ba), 0.0f, 1.0f);
	return math::length(pa - ba * h) - r;
}

float cone(const math::Vec3f &p, float angle, float h) {
	const float sinA = sinf(angle);
	const float cosA = cosf(angle);
	const math::Vec2f q = h * math::Vec2f(sinA / cosA, -1.0f);
	const math::Vec2f w(math::length(math::Vec2f(p.x, p.z)), p.y);
	const math::Vec2f a = w - q * math::clamp(math::dot(w, q) / math::dot(q, q), 0.0f, 1.0f);
	const math::Vec2f b = w - q * math::Vec2f(math::clamp(w.x / q.x, 0.0f, 1.0f), 1.0f);
	const float k = math::sign(q.y);
	const float d = math::min(math::dot(a, a), math::dot(b, b));
	const float s = math::max(k * (w.x * q.y - w.y * q.x), k * (w.y - q.y));
	return math::sqrt(d) * math::sign(s);
}

float roundCone(const math::Vec3f &p, float r1, float r2, float h) {
	const float b = (r1 - r2) / h;
	const float a = math::sqrt(1.0f - b * b);
	const math::Vec2f q(math::length(math::Vec2f(p.x, p.z)), p.y);
	const float k = math::dot(q, math::Vec2f(-b, a));
	if (k < 0.0f) {
		return math::length(q) - r1;
	}
	if (k > a * h) {
		return math::length(q - math::Vec2f(0.0f, h)) - r2;
	}
	return math::dot(q, math::Vec2f(a, b)) - r1;
}

float plane(const math::Vec3f &p, const math::Vec3f &n, float h) {
	return math::dot(p, n) + h;
}

float octahedron(const math::Vec3f &p, float s) {
	math::Vec3f ap = math::abs(p);
	const float m = ap.x + ap.y + ap.z - s;
	math::Vec3f q;
	if (3.0f * ap.x < m) {
		q = ap;
	} else if (3.0f * ap.y < m) {
		q = math::Vec3f(ap.y, ap.z, ap.x);
	} else if (3.0f * ap.z < m) {
		q = math::Vec3f(ap.z, ap.x, ap.y);
	} else {
		return m * 0.57735027f;
	}
	const float k = math::clamp(0.5f * (q.z - q.y + s), 0.0f, s);
	return math::length(math::Vec3f(q.x, q.y - s + k, q.z - k));
}

float pyramid(const math::Vec3f &p, float h) {
	const float m2 = h * h + 0.25f;
	math::Vec3f ap(math::abs(p.x), p.y, math::abs(p.z));
	if (ap.z > ap.x) {
		float tmp = ap.x;
		ap.x = ap.z;
		ap.z = tmp;
	}
	ap.x -= 0.5f;
	ap.z -= 0.5f;
	const math::Vec3f q(ap.z, h * ap.y - 0.5f * ap.x, h * ap.x + 0.5f * ap.y);
	const float s = math::max(-q.x, 0.0f);
	const float t = math::clamp((q.y - 0.5f * ap.z) / (m2 + 0.25f), 0.0f, 1.0f);
	const float a = m2 * (q.x + s) * (q.x + s) + q.y * q.y;
	const float b = m2 * (q.x + 0.5f * t) * (q.x + 0.5f * t) + (q.y - m2 * t) * (q.y - m2 * t);
	const float d2 = math::min(q.y, -q.x * m2 - q.y * 0.5f) > 0.0f ? 0.0f : math::min(a, b);
	return math::sqrt((d2 + q.z * q.z) / m2) * math::sign(math::max(q.z, -p.y));
}

float ellipsoid(const math::Vec3f &p, const math::Vec3f &r) {
	const float k0 = math::length(p / r);
	const float k1 = math::length(p / (r * r));
	return k0 * (k0 - 1.0f) / k1;
}

} // namespace sdf
} // namespace math
