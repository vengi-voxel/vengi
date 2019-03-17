/**
 * @file
 */

#include "Region.h"
#include "core/Common.h"

namespace voxel {

const Region Region::MaxRegion = Region(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());

glm::ivec3 Region::moveInto(int32_t x, int32_t y, int32_t z) const {
	const glm::ivec3& size = getDimensionsInVoxels();
	const glm::ivec3& mins = getLowerCorner();
	int32_t nx = x - mins.x;
	int32_t ny = y - mins.y;
	int32_t nz = z - mins.z;
	while (nx < mins.x) {
		nx += size.x;
	}
	while (ny < mins.y) {
		ny += size.y;
	}
	while (nz < mins.z) {
		nz += size.z;
	}
	const int32_t ox = mins.x + (nx % size.x);
	const int32_t oy = mins.y + (ny % size.y);
	const int32_t oz = mins.z + (nz % size.z);
	core_assert_msg(containsPoint(ox, oy, oz),
			"shifted(%i:%i:%i) is outside the valid region for pos(%i:%i:%i), size(%i:%i:%i), mins(%i:%i:%i)",
			ox, oy, oz, x, y, z, size.x, size.y, size.z, getLowerX(), getLowerY(), getLowerZ());
	return glm::ivec3(ox, oy, oz);
}

}
