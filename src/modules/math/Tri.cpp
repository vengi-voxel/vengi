/**
 * @file
 */

#include "Tri.h"
#include "color/Color.h"
#include <glm/ext/scalar_common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/type_aligned.hpp>

namespace math {

bool Tri::flat() const {
	const float eps = 0.00001f;
	const glm::bvec3 &zerocheck = glm::epsilonEqual(normal(), glm::zero<glm::vec3>(), eps);
	// if the normal of two components is zero
	return (int)zerocheck[0] + (int)zerocheck[1] + (int)zerocheck[2] == 2;
}

void Tri::setColor(const glm::vec4 &c) {
	setColor(color::Color::getRGBA(c));
}

glm::vec3 Tri::normal() const {
	return glm::cross(vertex1() - vertex0(), vertex2() - vertex0());
}

float Tri::area() const {
	return glm::length(normal()) / 2;
}

glm::ivec3 Tri::roundedMins() const {
	const glm::ivec3 intVert0 = glm::round(vertex0());
	const glm::ivec3 intVert1 = glm::round(vertex1());
	const glm::ivec3 intVert2 = glm::round(vertex2());
	return glm::min(intVert0, glm::min(intVert1, intVert2));
}

glm::ivec3 Tri::roundedMaxs() const {
	const glm::ivec3 intVert0 = glm::round(vertex0());
	const glm::ivec3 intVert1 = glm::round(vertex1());
	const glm::ivec3 intVert2 = glm::round(vertex2());
	return glm::max(intVert0, glm::max(intVert1, intVert2));
}

glm::vec3 Tri::mins() const {
	return glm::min(vertex0(), glm::min(vertex1(), vertex2()));
}

glm::vec3 Tri::maxs() const {
	return glm::max(vertex0(), glm::max(vertex1(), vertex2()));
}

glm::vec3 Tri::calculateBarycentric(const glm::vec3 &pos) const {
	const glm::aligned_vec4 v0(vertex0() - vertex2(), 0.0f);
	const glm::aligned_vec4 v1(vertex1() - vertex2(), 0.0f);
	const glm::aligned_vec4 v2(pos - vertex2(), 0.0f);
	const float d00 = glm::dot(v0, v0);
	const float d01 = glm::dot(v0, v1);
	const float d11 = glm::dot(v1, v1);
	const float r0 = glm::dot(v0, v2);
	const float r1 = glm::dot(v1, v2);
	const float det = d00 * d11 - d01 * d01;
	// Check for degenerated triangle
	if (det < glm::epsilon<float>()) {
		// Check if the triangle is degenerate and lies on a line
		if (d00 < glm::epsilon<float>()) {
			// vertex0() and vertex2() are the same point
			if (d11 < glm::epsilon<float>()) {
				// All vertices are the same point
				return glm::vec3(1.0f, 0.0f, 0.0f); // All barycentric weight goes to vertex0()
			} else {
				// Collapse to a line between vertex1() and vertex2()
				float t = glm::clamp(r1 / d11, 0.0f, 1.0f);
				return glm::vec3(0.0f, t, 1.0f - t); // Return interpolation on the line
			}
		} else {
			// Collapse to a line between vertex0() and vertex2()
			float t = glm::clamp(r0 / d00, 0.0f, 1.0f);
			return glm::vec3(t, 0.0f, 1.0f - t); // Return interpolation on the line
		}
	}
	const float invDet = 1.0f / det;
	const float b0 = (d11 * r0 - d01 * r1) * invDet;
	const float b1 = (d00 * r1 - d01 * r0) * invDet;
	const float b2 = 1.0f - b0 - b1;
	return glm::vec3(b0, b1, b2);
}

void Tri::scaleVertices(float scale) {
	for (int i = 0; i < 3; ++i) {
		_vertices[i].pos *= scale;
	}
}

void Tri::scaleVertices(const glm::vec3 &scale) {
	for (int i = 0; i < 3; ++i) {
		_vertices[i].pos *= scale;
	}
}

} // namespace math
