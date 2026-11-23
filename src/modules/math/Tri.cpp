/**
 * @file
 */

#include "Tri.h"
#include "color/ColorUtil.h"
#include "math/AlignedVec4f.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/BVec3.h"
#include "math/Scalar.h"
#include "math/Conversions.h"

namespace math {

bool Tri::flat() const {
	const float eps = 0.00001f;
	const math::BVec3 &zerocheck = math::epsilonEqual(normal(), math::Vec3f(0.0f), eps);
	// if the normal of two components is zero
	return (int)zerocheck[0] + (int)zerocheck[1] + (int)zerocheck[2] == 2;
}

void Tri::setColor(const math::Vec4f &c) {
	setColor(color::getRGBA(c));
}

math::Vec3f Tri::normal() const {
	return math::cross(vertex1() - vertex0(), vertex2() - vertex0());
}

float Tri::area() const {
	return math::length(normal()) / 2;
}

math::Vec3i Tri::roundedMins() const {
	const math::Vec3i intVert0 = math::round(vertex0());
	const math::Vec3i intVert1 = math::round(vertex1());
	const math::Vec3i intVert2 = math::round(vertex2());
	return math::min(intVert0, math::min(intVert1, intVert2));
}

math::Vec3i Tri::roundedMaxs() const {
	const math::Vec3i intVert0 = math::round(vertex0());
	const math::Vec3i intVert1 = math::round(vertex1());
	const math::Vec3i intVert2 = math::round(vertex2());
	return math::max(intVert0, math::max(intVert1, intVert2));
}

math::Vec3f Tri::mins() const {
	return math::min(vertex0(), math::min(vertex1(), vertex2()));
}

math::Vec3f Tri::maxs() const {
	return math::max(vertex0(), math::max(vertex1(), vertex2()));
}

math::Vec3f Tri::calculateBarycentric(const math::Vec3f &pos) const {
	const math::AlignedVec4f v0(vertex0() - vertex2(), 0.0f);
	const math::AlignedVec4f v1(vertex1() - vertex2(), 0.0f);
	const math::AlignedVec4f v2(pos - vertex2(), 0.0f);
	const float d00 = math::dot(v0, v0);
	const float d01 = math::dot(v0, v1);
	const float d11 = math::dot(v1, v1);
	const float r0 = math::dot(v0, v2);
	const float r1 = math::dot(v1, v2);
	const float det = d00 * d11 - d01 * d01;
	// Check for degenerated triangle
	if (det < math::epsilonf) {
		// Check if the triangle is degenerate and lies on a line
		if (d00 < math::epsilonf) {
			// vertex0() and vertex2() are the same point
			if (d11 < math::epsilonf) {
				// All vertices are the same point
				return math::Vec3f(1.0f, 0.0f, 0.0f); // All barycentric weight goes to vertex0()
			} else {
				// Collapse to a line between vertex1() and vertex2()
				float t = math::clamp(r1 / d11, 0.0f, 1.0f);
				return math::Vec3f(0.0f, t, 1.0f - t); // Return interpolation on the line
			}
		} else {
			// Collapse to a line between vertex0() and vertex2()
			float t = math::clamp(r0 / d00, 0.0f, 1.0f);
			return math::Vec3f(t, 0.0f, 1.0f - t); // Return interpolation on the line
		}
	}
	const float invDet = 1.0f / det;
	const float b0 = (d11 * r0 - d01 * r1) * invDet;
	const float b1 = (d00 * r1 - d01 * r0) * invDet;
	const float b2 = 1.0f - b0 - b1;
	return math::Vec3f(b0, b1, b2);
}

void Tri::scaleVertices(float scale) {
	for (int i = 0; i < 3; ++i) {
		_vertices[i].pos *= scale;
	}
}

void Tri::scaleVertices(const math::Vec3f &scale) {
	for (int i = 0; i < 3; ++i) {
		_vertices[i].pos *= scale;
	}
}

} // namespace math
