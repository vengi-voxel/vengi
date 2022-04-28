/**
 * @file
 */

#include "VolumeSplitter.h"
#include "core/Common.h"
#include "core/Log.h"
#include "voxelutil/VoxelUtil.h"

namespace voxelutil {

void splitVolume(const voxel::RawVolume *volume, const glm::ivec3 &maxSize,
				 core::DynamicArray<voxel::RawVolume *> &rawVolumes) {
	const voxel::Region &region = volume->region();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();

	const glm::ivec3 step = glm::min(maxs, maxSize);
	Log::debug("split region: %s", region.toString().c_str());
	for (int y = mins.y; y <= maxs.y; y += step.y) {
		for (int z = mins.z; z <= maxs.z; z += step.z) {
			for (int x = mins.x; x <= maxs.x; x += step.x) {
				const glm::ivec3 innerMins(x, y, z);
				const glm::ivec3 innerMaxs = innerMins + maxSize - 1;
				const voxel::Region innerRegion(innerMins, innerMaxs);
				voxel::RawVolume *copy = new voxel::RawVolume(innerRegion);
				if (!voxelutil::copy(*volume, innerRegion, *copy, innerRegion)) {
					Log::debug("- skip empty %s", innerRegion.toString().c_str());
					delete copy;
					continue;
				} else {
					Log::debug("- split %s", innerRegion.toString().c_str());
				}
				rawVolumes.push_back(copy);
			}
		}
	}
}

} // namespace voxelutil
