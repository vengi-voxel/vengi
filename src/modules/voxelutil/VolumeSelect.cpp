/**
 * @file
 */

#include "VolumeSelect.h"
#include "voxel/RawVolume.h"

namespace voxelutil {

voxel::Region regionForFlag(const voxel::RawVolume &volume, uint8_t flag) {
	voxel::Region result = voxel::Region::InvalidRegion;
	const voxel::Region &region = volume.region();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	for (int32_t z = mins.z; z <= maxs.z; ++z) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			for (int32_t x = mins.x; x <= maxs.x; ++x) {
				const voxel::Voxel &voxel = volume.voxel(x, y, z);
				if ((voxel.getFlags() & flag) != 0) {
					if (result.isValid()) {
						result.accumulate(x, y, z);
					} else {
						result = voxel::Region(x, y, z, x, y, z);
					}
				}
			}
		}
	}
	return result;
}

bool lassoContains(const core::DynamicArray<glm::ivec3> &path, int pu, int pv, int uAxis, int vAxis) {
	const int n = (int)path.size();
	bool inside = false;
	for (int i = 0, j = n - 1; i < n; j = i++) {
		const double xi = (double)path[i][uAxis];
		const double yi = (double)path[i][vAxis];
		const double xj = (double)path[j][uAxis];
		const double yj = (double)path[j][vAxis];
		const double px = (double)pu;
		const double py = (double)pv;
		if (((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) {
			inside = !inside;
		}
	}
	return inside;
}

} // namespace voxelutil
