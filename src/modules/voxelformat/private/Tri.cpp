/**
 * @file
 */

#include "Tri.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace voxelformat {

glm::vec3 Tri::normal() const {
	return glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);
}

float Tri::area() const {
	return glm::length(normal()) / 2;
}

glm::vec3 Tri::mins() const {
	glm::vec3 v;
	for (int i = 0; i < 3; ++i) {
		v[i] = core_min(vertices[0][i], core_min(vertices[1][i], vertices[2][i]));
	}
	return v;
}

glm::vec3 Tri::maxs() const {
	glm::vec3 v;
	for (int i = 0; i < 3; ++i) {
		v[i] = core_max(vertices[0][i], core_max(vertices[1][i], vertices[2][i]));
	}
	return v;
}

core::RGBA Tri::colorAt(const glm::vec2 &uv) const {
	if (texture) {
		return texture->colorAt(uv, wrapS, wrapT);
	}
	return color;
}

// Sierpinski gasket with keeping the middle
void Tri::subdivide(Tri out[4]) const {
	const glm::vec3 midv[]{glm::mix(vertices[0], vertices[1], 0.5f), glm::mix(vertices[1], vertices[2], 0.5f),
						   glm::mix(vertices[2], vertices[0], 0.5f)};
	const glm::vec2 miduv[]{glm::mix(uv[0], uv[1], 0.5f), glm::mix(uv[1], uv[2], 0.5f), glm::mix(uv[2], uv[0], 0.5f)};

	// the subdivided new three triangles
	out[0] = Tri{{vertices[0], midv[0], midv[2]}, {uv[0], miduv[0], miduv[2]}, texture, color};
	out[1] = Tri{{vertices[1], midv[1], midv[0]}, {uv[1], miduv[1], miduv[0]}, texture, color};
	out[2] = Tri{{vertices[2], midv[2], midv[1]}, {uv[2], miduv[2], miduv[1]}, texture, color};
	// keep the middle
	out[3] = Tri{{midv[0], midv[1], midv[2]}, {miduv[0], miduv[1], miduv[2]}, texture, color};
}

} // namespace voxelformat
