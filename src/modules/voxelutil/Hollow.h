/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "core/ProgressScope.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

template<class VOLUME>
inline void hollow(VOLUME &volume) {
	core::DynamicArray<glm::ivec3> toHollow;
	toHollow.reserve(volume.region().voxels());
	core::IProgress &progress = core::currentProgress();
	progress.setProgress(0.0f);
	voxelutil::visitInvisibleVolume(volume, [&toHollow](int x, int y, int z, const voxel::Voxel &voxel) {
		toHollow.emplace_back(x, y, z);
	});
	progress.setProgress(0.5f);
	const int n = (int)toHollow.size();
	const int progressStep = 1024;
	for (int i = 0; i < n; ++i) {
		volume.setVoxel(toHollow[i], voxel::Voxel());
		if (i % progressStep == 0) {
			progress.setProgress(0.5f + 0.5f * ((float)(i + 1) / (float)core_max(1, n)));
		}
	}
	progress.setProgress(1.0f);
}

} // namespace voxelutil
