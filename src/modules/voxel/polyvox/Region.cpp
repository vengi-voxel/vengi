/**
 * @file
 */

#include "Region.h"
#include "core/Common.h"

namespace voxel {

const Region Region::MaxRegion = Region(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());

glm::ivec3 Region::moveInto(int32_t x, int32_t y, int32_t z) const {
	const glm::ivec3& size = getDimensionsInVoxels();
	int32_t nx = x - getLowerX();
	int32_t ny = y - getLowerY();
	int32_t nz = z - getLowerZ();
	while (nx < 0) {
		nx += getWidthInVoxels();
	}
	while (ny < 0) {
		ny += getHeightInVoxels();
	}
	while (nz < 0) {
		nz += getDepthInVoxels();
	}
	const int32_t ox = nx % size.x;
	const int32_t oy = ny % size.y;
	const int32_t oz = nz % size.z;
	core_assert_msg(containsPoint(ox, oy, oz),
			"shifted(%i:%i:%i) is outside the valid region for pos(%i:%i:%i), size(%i:%i:%i), mins(%i:%i:%i)",
			ox, oy, oz, x, y, z, size.x, size.y, size.z, getLowerX(), getLowerY(), getLowerZ());
	return glm::ivec3(ox, oy, oz);
}

}
