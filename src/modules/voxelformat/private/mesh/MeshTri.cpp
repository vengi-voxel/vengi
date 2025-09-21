/**
 * @file
 */

#include "MeshTri.h"
#include <glm/ext/scalar_common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/type_aligned.hpp>

namespace voxelformat {

void MeshTri::setUVs(const glm::vec2 &uv1, const glm::vec2 &uv2, const glm::vec2 &uv3) {
	_uv[0] = uv1;
	_uv[1] = uv2;
	_uv[2] = uv3;
}

glm::vec2 MeshTri::centerUV() const {
	return (uv0() + uv1() + uv2()) / 3.0f;
}

int MeshTri::subdivideTriCount(size_t maxPerTriangle) const {
	const float maxSide = maxSideLength();
	if (maxSide <= 1.0f) {
		return 1;
	}
	int d = (int)glm::ceil(glm::log2(maxSide));
	if (d < 0) {
		d = 0;
	}
	if (d > 16) {
		d = 16; // match depth limit in subdivideTri
	}
	size_t trisCount = 1;
	for (int k = 0; k < d; ++k) {
		// multiply by 4 each level, but cap to avoid overflow
		if (trisCount > maxPerTriangle / 4) {
			trisCount = maxPerTriangle;
			break;
		}
		trisCount *= 4;
	}
	return trisCount;
}

float MeshTri::maxSideLength() const {
	const glm::vec3 &v0 = vertex0();
	const glm::vec3 &v1 = vertex1();
	const glm::vec3 &v2 = vertex2();
	const float s0 = glm::length(v0 - v1);
	const float s1 = glm::length(v1 - v2);
	const float s2 = glm::length(v2 - v0);
	const float maxSide = glm::max(glm::max(s0, s1), s2);
	return maxSide;
}

// https://en.wikipedia.org/wiki/Barycentric_coordinate_system
bool MeshTri::calcUVs(const glm::vec3 &pos, glm::vec2 &outUV) const {
	const glm::vec3 &b = calculateBarycentric(pos);

	// Check if barycentric coordinates are within [0, 1]
	if (b.x >= 0.0f && b.x <= 1.0f && b.y >= 0.0f && b.y <= 1.0f && b.z >= 0.0f && b.z <= 1.0f) {
		// Interpolate UVs using barycentric coordinates
		outUV = b.x * uv0() + b.y * uv1() + b.z * uv2();
		return true;
	}

	return false;
}

} // namespace voxelformat
