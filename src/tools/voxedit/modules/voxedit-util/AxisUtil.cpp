/**
 * @file
 */

#include "AxisUtil.h"

namespace voxedit {

void updateShapeBuilderForPlane(video::ShapeBuilder& shapeBuilder, const voxel::Region& region, bool mirror, const glm::ivec3& pos, math::Axis axis, const glm::vec4& color) {
	const int index = mirror ? getIndexForMirrorAxis(axis) : getIndexForAxis(axis);
	glm::vec3 mins = region.getLowerCorner();
	glm::vec3 maxs = region.getUpperCorner();
	mins[index] = maxs[index] = pos[index];
	const glm::vec3& ll = mins;
	const glm::vec3& ur = maxs;
	glm::vec3 ul;
	glm::vec3 lr;
	if (axis == math::Axis::Y) {
		ul = glm::vec3(mins.x, mins.y, maxs.z);
		lr = glm::vec3(maxs.x, maxs.y, mins.z);
	} else {
		ul = glm::vec3(mins.x, maxs.y, mins.z);
		lr = glm::vec3(maxs.x, mins.y, maxs.z);
	}
	std::vector<glm::vec3> vecs({ll, ul, ur, lr});
	// lower left (0), upper left (1), upper right (2)
	// lower left (0), upper right (2), lower right (3)
	const std::vector<uint32_t> indices { 0, 1, 2, 0, 2, 3, 2, 1, 0, 3, 2, 0 };
	shapeBuilder.clear();
	shapeBuilder.setColor(color);
	shapeBuilder.geom(vecs, indices);
}

}
