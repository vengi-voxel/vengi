/**
 * @file
 * @brief Signed distance functions based on https://iquilezles.org/articles/distfunctions/
 */

#include "math/SDF.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <math.h>

namespace math {
namespace sdf {

float goxSphere(const glm::vec3 &p, const glm::vec3 &s) {
	const float d = glm::length(p);
	if (d == 0.0f) {
		return glm::max(s.x, glm::max(s.y, s.z));
	}
	return d / glm::length(p / s) - d;
}

float goxCube(const glm::vec3 &p, const glm::vec3 &s) {
	if (glm::any(glm::lessThan(p, -s)) || glm::any(glm::greaterThanEqual(p, s))) {
		return -1.0f;
	}
	const glm::vec3 ap = glm::abs(p);
	const glm::vec3 ratio = ap / s;
	int axis = 0;
	if (ratio.y > ratio[axis]) {
		axis = 1;
	}
	if (ratio.z > ratio[axis]) {
		axis = 2;
	}
	return s[axis] - ap[axis];
}

float goxCylinder(const glm::vec3 &p, const glm::vec3 &s) {
	const glm::vec2 pxy(p.x, p.y);
	const glm::vec2 sxy(s.x, s.y);
	const float d = glm::length(pxy);
	const float rz = s.z - glm::abs(p.z);
	if (d == 0.0f) {
		return glm::min(rz, glm::max(s.x, glm::max(s.y, s.z)));
	}
	return glm::min(rz, d / glm::length(pxy / sxy) - d);
}

float sphere(const glm::vec3 &p, float r) {
	return glm::length(p) - r;
}

float box(const glm::vec3 &p, const glm::vec3 &b) {
	const glm::vec3 q = glm::abs(p) - b;
	return glm::length(glm::max(q, 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f);
}

float roundBox(const glm::vec3 &p, const glm::vec3 &b, float r) {
	const glm::vec3 q = glm::abs(p) - b + r;
	return glm::length(glm::max(q, 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f) - r;
}

float boxFrame(const glm::vec3 &p, const glm::vec3 &b, float e) {
	glm::vec3 ap = glm::abs(p) - b;
	const glm::vec3 q = glm::abs(ap + e) - e;
	return glm::min(glm::min(
		glm::length(glm::max(glm::vec3(ap.x, q.y, q.z), 0.0f)) + glm::min(glm::max(ap.x, glm::max(q.y, q.z)), 0.0f),
		glm::length(glm::max(glm::vec3(q.x, ap.y, q.z), 0.0f)) + glm::min(glm::max(q.x, glm::max(ap.y, q.z)), 0.0f)),
		glm::length(glm::max(glm::vec3(q.x, q.y, ap.z), 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, ap.z)), 0.0f));
}

float torus(const glm::vec3 &p, float majorRadius, float minorRadius) {
	const glm::vec2 q(glm::length(glm::vec2(p.x, p.z)) - majorRadius, p.y);
	return glm::length(q) - minorRadius;
}

float capsule(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, float r) {
	const glm::vec3 pa = p - a;
	const glm::vec3 ba = b - a;
	const float h = glm::clamp(glm::dot(pa, ba) / glm::dot(ba, ba), 0.0f, 1.0f);
	return glm::length(pa - ba * h) - r;
}

float cone(const glm::vec3 &p, float angle, float h) {
	const float sinA = sinf(angle);
	const float cosA = cosf(angle);
	const glm::vec2 q = h * glm::vec2(sinA / cosA, -1.0f);
	const glm::vec2 w(glm::length(glm::vec2(p.x, p.z)), p.y);
	const glm::vec2 a = w - q * glm::clamp(glm::dot(w, q) / glm::dot(q, q), 0.0f, 1.0f);
	const glm::vec2 b = w - q * glm::vec2(glm::clamp(w.x / q.x, 0.0f, 1.0f), 1.0f);
	const float k = glm::sign(q.y);
	const float d = glm::min(glm::dot(a, a), glm::dot(b, b));
	const float s = glm::max(k * (w.x * q.y - w.y * q.x), k * (w.y - q.y));
	return glm::sqrt(d) * glm::sign(s);
}

float roundCone(const glm::vec3 &p, float r1, float r2, float h) {
	const float b = (r1 - r2) / h;
	const float a = glm::sqrt(1.0f - b * b);
	const glm::vec2 q(glm::length(glm::vec2(p.x, p.z)), p.y);
	const float k = glm::dot(q, glm::vec2(-b, a));
	if (k < 0.0f) {
		return glm::length(q) - r1;
	}
	if (k > a * h) {
		return glm::length(q - glm::vec2(0.0f, h)) - r2;
	}
	return glm::dot(q, glm::vec2(a, b)) - r1;
}

float plane(const glm::vec3 &p, const glm::vec3 &n, float h) {
	return glm::dot(p, n) + h;
}

float octahedron(const glm::vec3 &p, float s) {
	glm::vec3 ap = glm::abs(p);
	const float m = ap.x + ap.y + ap.z - s;
	glm::vec3 q;
	if (3.0f * ap.x < m) {
		q = ap;
	} else if (3.0f * ap.y < m) {
		q = glm::vec3(ap.y, ap.z, ap.x);
	} else if (3.0f * ap.z < m) {
		q = glm::vec3(ap.z, ap.x, ap.y);
	} else {
		return m * 0.57735027f;
	}
	const float k = glm::clamp(0.5f * (q.z - q.y + s), 0.0f, s);
	return glm::length(glm::vec3(q.x, q.y - s + k, q.z - k));
}

float pyramid(const glm::vec3 &p, float h) {
	const float m2 = h * h + 0.25f;
	glm::vec3 ap(glm::abs(p.x), p.y, glm::abs(p.z));
	if (ap.z > ap.x) {
		float tmp = ap.x;
		ap.x = ap.z;
		ap.z = tmp;
	}
	ap.x -= 0.5f;
	ap.z -= 0.5f;
	const glm::vec3 q(ap.z, h * ap.y - 0.5f * ap.x, h * ap.x + 0.5f * ap.y);
	const float s = glm::max(-q.x, 0.0f);
	const float t = glm::clamp((q.y - 0.5f * ap.z) / (m2 + 0.25f), 0.0f, 1.0f);
	const float a = m2 * (q.x + s) * (q.x + s) + q.y * q.y;
	const float b = m2 * (q.x + 0.5f * t) * (q.x + 0.5f * t) + (q.y - m2 * t) * (q.y - m2 * t);
	const float d2 = glm::min(q.y, -q.x * m2 - q.y * 0.5f) > 0.0f ? 0.0f : glm::min(a, b);
	return glm::sqrt((d2 + q.z * q.z) / m2) * glm::sign(glm::max(q.z, -p.y));
}

float ellipsoid(const glm::vec3 &p, const glm::vec3 &r) {
	const float k0 = glm::length(p / r);
	const float k1 = glm::length(p / (r * r));
	return k0 * (k0 - 1.0f) / k1;
}

} // namespace sdf
} // namespace math
