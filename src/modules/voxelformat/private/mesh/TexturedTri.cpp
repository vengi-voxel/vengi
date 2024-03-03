/**
 * @file
 */

#include "TexturedTri.h"
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

glm::vec2 TexturedTri::centerUV() const {
	return (uv[0] + uv[1] + uv[2]) / 3.0f;
}

core::RGBA TexturedTri::centerColor() const {
	if (texture) {
		const glm::vec2 &c = centerUV();
		return texture->colorAt(c, wrapS, wrapT);
	}
	return core::RGBA::mix(core::RGBA::mix(color[0], color[1]), color[2]);
}

core::RGBA TexturedTri::colorAt(const glm::vec2 &inputuv) const {
	if (texture) {
		return texture->colorAt(inputuv, wrapS, wrapT);
	}
	return core::RGBA::mix(core::RGBA::mix(color[0], color[1]), color[2]);
}

// Sierpinski gasket with keeping the middle
void TexturedTri::subdivide(TexturedTri out[4]) const {
	const glm::vec3 midv[]{glm::mix(vertices[0], vertices[1], 0.5f), glm::mix(vertices[1], vertices[2], 0.5f),
						   glm::mix(vertices[2], vertices[0], 0.5f)};
	const glm::vec2 miduv[]{glm::mix(uv[0], uv[1], 0.5f), glm::mix(uv[1], uv[2], 0.5f), glm::mix(uv[2], uv[0], 0.5f)};
	const core::RGBA midc[]{core::RGBA::mix(color[0], color[1]), core::RGBA::mix(color[1], color[2]),
							core::RGBA::mix(color[2], color[0])};

	// the subdivided new three triangles
	out[0] = TexturedTri{
		{vertices[0], midv[0], midv[2]}, {uv[0], miduv[0], miduv[2]}, texture, {color[0], midc[0], midc[2]}};
	out[1] = TexturedTri{
		{vertices[1], midv[1], midv[0]}, {uv[1], miduv[1], miduv[0]}, texture, {color[1], midc[1], midc[0]}};
	out[2] = TexturedTri{
		{vertices[2], midv[2], midv[1]}, {uv[2], miduv[2], miduv[1]}, texture, {color[2], midc[2], midc[1]}};
	// keep the middle
	out[3] =
		TexturedTri{{midv[0], midv[1], midv[2]}, {miduv[0], miduv[1], miduv[2]}, texture, {midc[0], midc[1], midc[2]}};
}

// https://en.wikipedia.org/wiki/Barycentric_coordinate_system
bool TexturedTri::calcUVs(const glm::vec3 &pos, glm::vec2 &outUV) const {
	const glm::vec3 &b = calculateBarycentric(pos);

	// Check if barycentric coordinates are within [0, 1]
	if (b.x >= 0.0f && b.x <= 1.0f && b.y >= 0.0f && b.y <= 1.0f && b.z >= 0.0f && b.z <= 1.0f) {
		// Interpolate UVs using barycentric coordinates
		outUV = b.x * uv[0] + b.y * uv[1] + b.z * uv[2];
		return true;
	}

	return false;
}

} // namespace voxelformat
