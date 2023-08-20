/**
 * @file
 */

#include "Tri.h"
#include <glm/ext/scalar_common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/type_aligned.hpp>

namespace voxelformat {

bool Tri::flat() const {
	const float eps = glm::epsilon<float>();
	const glm::bvec3 &zerocheck = glm::epsilonEqual(normal(), glm::zero<glm::vec3>(), eps);
	// if the normal of two components is zero
	return (int)zerocheck[0] + (int)zerocheck[1] + (int)zerocheck[2] == 2;
}

glm::vec3 Tri::normal() const {
	return glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);
}

float Tri::area() const {
	return glm::length(normal()) / 2;
}

glm::ivec3 Tri::roundedMins() const {
	const glm::ivec3 intVert0 = glm::round(vertices[0]);
	const glm::ivec3 intVert1 = glm::round(vertices[1]);
	const glm::ivec3 intVert2 = glm::round(vertices[2]);
	return glm::min(intVert0, glm::min(intVert1, intVert2));
}

glm::ivec3 Tri::roundedMaxs() const {
	const glm::ivec3 intVert0 = glm::round(vertices[0]);
	const glm::ivec3 intVert1 = glm::round(vertices[1]);
	const glm::ivec3 intVert2 = glm::round(vertices[2]);
	return glm::max(intVert0, glm::max(intVert1, intVert2));
}

glm::vec3 Tri::mins() const {
	return glm::min(vertices[0], glm::min(vertices[1], vertices[2]));
}

glm::vec3 Tri::maxs() const {
	return glm::max(vertices[0], glm::max(vertices[1], vertices[2]));
}

core::RGBA Tri::centerColor() const {
	if (texture) {
		return texture->colorAt(centerUV(), wrapS, wrapT);
	}
	return core::RGBA::mix(core::RGBA::mix(color[0], color[1]), color[2]);
}

core::RGBA Tri::colorAt(const glm::vec2 &inputuv) const {
	if (texture) {
		return texture->colorAt(inputuv, wrapS, wrapT);
	}
	return core::RGBA::mix(core::RGBA::mix(color[0], color[1]), color[2]);
}

// Sierpinski gasket with keeping the middle
void Tri::subdivide(Tri out[4]) const {
	const glm::vec3 midv[]{glm::mix(vertices[0], vertices[1], 0.5f), glm::mix(vertices[1], vertices[2], 0.5f),
						   glm::mix(vertices[2], vertices[0], 0.5f)};
	const glm::vec2 miduv[]{glm::mix(uv[0], uv[1], 0.5f), glm::mix(uv[1], uv[2], 0.5f), glm::mix(uv[2], uv[0], 0.5f)};
	const core::RGBA midc[]{core::RGBA::mix(color[0], color[1]), core::RGBA::mix(color[1], color[2]),
							core::RGBA::mix(color[2], color[0])};

	// the subdivided new three triangles
	out[0] = Tri{{vertices[0], midv[0], midv[2]}, {uv[0], miduv[0], miduv[2]}, texture, {color[0], midc[0], midc[2]}};
	out[1] = Tri{{vertices[1], midv[1], midv[0]}, {uv[1], miduv[1], miduv[0]}, texture, {color[1], midc[1], midc[0]}};
	out[2] = Tri{{vertices[2], midv[2], midv[1]}, {uv[2], miduv[2], miduv[1]}, texture, {color[2], midc[2], midc[1]}};
	// keep the middle
	out[3] = Tri{{midv[0], midv[1], midv[2]}, {miduv[0], miduv[1], miduv[2]}, texture, {midc[0], midc[1], midc[2]}};
}

glm::vec3 Tri::calculateBarycentric(const glm::vec3 &pos) const {
	const glm::vec4 v0(vertices[0] - vertices[2], 0.0f);
	const glm::vec4 v1(vertices[1] - vertices[2], 0.0f);
	const glm::vec4 v2(pos - vertices[2], 0.0f);
	const float d00 = glm::dot(v0, v0);
	const float d01 = glm::dot(v0, v1);
	const float d11 = glm::dot(v1, v1);
	const float r0 = glm::dot(v0, v2);
	const float r1 = glm::dot(v1, v2);
	const float det = d00 * d11 - d01 * d01;
	// TODO: only works for non-degenerated triangles
	if (det < glm::epsilon<float>()) {
		return glm::vec3(-1.0f);
	}
	const float invDet = 1.0f / det;
	const float b0 = (d11 * r0 - d01 * r1) * invDet;
	const float b1 = (d00 * r1 - d01 * r0) * invDet;
	const float b2 = 1.0f - b0 - b1;
	return glm::vec3(b0, b1, b2);
}

// https://en.wikipedia.org/wiki/Barycentric_coordinate_system
bool Tri::calcUVs(const glm::vec3 &pos, glm::vec2 &outUV) const {
	const glm::vec3 &b = calculateBarycentric(pos);

	// Check if barycentric coordinates are within [0, 1]
	if (b.x >= 0.0f && b.x <= 1.0f && b.y >= 0.0f && b.y <= 1.0f &&
		b.z >= 0.0f && b.z <= 1.0f) {
		// Interpolate UVs using barycentric coordinates
		outUV = b.x * uv[0] + b.y * uv[1] + b.z * uv[2];
		return true;
	}

	return false;
}

} // namespace voxelformat
