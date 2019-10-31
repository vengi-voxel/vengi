/**
 * @file
 */

#include "Region.h"
#include "core/Common.h"
#include "core/Log.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <stdint.h>
#include <limits>

namespace voxel {

const Region Region::MaxRegion = Region((std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::max)());
const Region Region::InvalidRegion = Region(0, -1);

std::string Region::toString() const {
	const std::string& regionStr = "region["
		"center(" + glm::to_string(getCentre()) + "), "
		"mins(" + glm::to_string(getLowerCorner()) + "), "
		"maxs(" + glm::to_string(getUpperCorner()) + ")"
	"]";
	return regionStr;
}

glm::ivec3 Region::moveInto(int32_t x, int32_t y, int32_t z) const {
	const glm::ivec3& size = getDimensionsInVoxels();
	const glm::ivec3& mins = getLowerCorner();
	const glm::ivec3& maxs = getUpperCorner();
	const int32_t ox = (x < 0 ? maxs.x : mins.x) + (x % size.x);
	const int32_t oy = (y < 0 ? maxs.y : mins.y) + (y % size.y);
	const int32_t oz = (z < 0 ? maxs.z : mins.z) + (z % size.z);
	core_assert_msg(containsPoint(ox, oy, oz),
			"shifted(%i:%i:%i) is outside the valid region for pos(%i:%i:%i), size(%i:%i:%i), mins(%i:%i:%i)",
			ox, oy, oz, x, y, z, size.x, size.y, size.z, getLowerX(), getLowerY(), getLowerZ());
	return glm::ivec3(ox, oy, oz);
}

void logRegion(const char *ctx, const voxel::Region& region) {
	Log::debug("%s: %s", ctx, region.toString().c_str());
}

int Region::voxels() const {
	return getWidthInVoxels() * getHeightInVoxels() * getDepthInVoxels();
}

}
