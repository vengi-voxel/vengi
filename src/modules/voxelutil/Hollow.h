/**
 * @file
 */

#pragma once

#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

template<class VOLUME>
inline void hollow(VOLUME &volume) {
	voxelutil::visitUndergroundVolume(volume, [&volume](int x, int y, int z, const voxel::Voxel &voxel) {
		volume.setVoxel(x, y, z, voxel::Voxel());
	});
}

} // namespace voxelutil
