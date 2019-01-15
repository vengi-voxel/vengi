/**
 * @file
 */

#include "Fill.h"

namespace voxedit {
namespace tool {

bool fill(voxel::RawVolume& target, const glm::ivec3& position, const math::Axis axis, const voxel::Voxel& voxel, bool overwrite, voxel::Region* modifiedRegion) {
	const voxel::Region& region = target.region();
	int zStart = region.getLowerZ();
	glm::ivec3 modifiedMins(std::numeric_limits<int>::max());
	glm::ivec3 modifiedMaxs(std::numeric_limits<int>::min());
	int cnt = 0;
	if ((axis & math::Axis::Z) != math::Axis::None) {
		zStart = position.y;
	}
	for (int32_t z = zStart; z <= region.getUpperZ(); ++z) {
		int yStart = region.getLowerY();
		if ((axis & math::Axis::Y) != math::Axis::None) {
			yStart = position.y;
		}
		for (int32_t y = yStart; y <= region.getUpperY(); ++y) {
			int xStart = region.getLowerX();
			if ((axis & math::Axis::X) != math::Axis::None) {
				xStart = position.x;
			}
			for (int32_t x = xStart; x <= region.getUpperX(); ++x) {
				if (overwrite || isAir(target.voxel(x, y, z).getMaterial())) {
					if (target.setVoxel(x, y, z, voxel)) {
						++cnt;
						modifiedMins.x = glm::min(modifiedMins.x, x);
						modifiedMins.y = glm::min(modifiedMins.y, y);
						modifiedMins.z = glm::min(modifiedMins.z, z);

						modifiedMaxs.x = glm::max(modifiedMaxs.x, x);
						modifiedMaxs.y = glm::max(modifiedMaxs.y, y);
						modifiedMaxs.z = glm::max(modifiedMaxs.z, z);
					}
				}
				if ((axis & math::Axis::X) != math::Axis::None) {
					break;
				}
			}
			if ((axis & math::Axis::Y) != math::Axis::None) {
				break;
			}
		}
		if ((axis & math::Axis::Z) != math::Axis::None) {
			break;
		}
	}
	if (cnt <= 0) {
		return false;
	}
	if (modifiedRegion != nullptr) {
		*modifiedRegion = voxel::Region(modifiedMins, modifiedMaxs);
	}
	return true;
}

bool aabb(voxel::RawVolume& target, const glm::ivec3& mins, const glm::ivec3& maxs, const voxel::Voxel& voxel, bool overwrite, voxel::Region* modifiedRegion) {
	glm::ivec3 modifiedMins(std::numeric_limits<int>::max());
	glm::ivec3 modifiedMaxs(std::numeric_limits<int>::min());
	int cnt = 0;
	for (int32_t z = mins.z; z <= maxs.z; ++z) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			for (int32_t x = mins.x; x <= maxs.x; ++x) {
				if (overwrite || isAir(target.voxel(x, y, z).getMaterial())) {
					if (target.setVoxel(x, y, z, voxel)) {
						++cnt;
						modifiedMins.x = glm::min(modifiedMins.x, x);
						modifiedMins.y = glm::min(modifiedMins.y, y);
						modifiedMins.z = glm::min(modifiedMins.z, z);

						modifiedMaxs.x = glm::max(modifiedMaxs.x, x);
						modifiedMaxs.y = glm::max(modifiedMaxs.y, y);
						modifiedMaxs.z = glm::max(modifiedMaxs.z, z);
					}
				}
			}
		}
	}
	if (cnt <= 0) {
		return false;
	}
	if (modifiedRegion != nullptr) {
		*modifiedRegion = voxel::Region(modifiedMins, modifiedMaxs);
	}
	return true;
}

}
}
