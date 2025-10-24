/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

template<class VOLUME>
inline void hollow(VOLUME &volume) {
	core::DynamicArray<glm::ivec3> toHollow;
	toHollow.reserve(volume.region().voxels());
	voxelutil::visitUndergroundVolume(volume, [&toHollow](int x, int y, int z, const voxel::Voxel &voxel) {
		toHollow.emplace_back(x, y, z);
	});
	for (const glm::ivec3 &pos : toHollow) {
		volume.setVoxel(pos, voxel::Voxel());
	}
}

} // namespace voxelutil
