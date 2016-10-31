#pragma once

#include "RawVolume.h"

namespace voxel {

int mergeRawVolumes(RawVolume* destination, const RawVolume* source, const Voxel& emptyVoxel) {
	core_trace_scoped(MergeRawVolumes);
	int cnt = 0;
	RawVolume::Sampler srcSampler(source);
	RawVolume::Sampler dstSampler(destination);
	const Region& destRegion = destination->getEnclosingRegion();
	core_assert_always(destRegion == source->getEnclosingRegion());
	const int32_t depth = destRegion.getDepthInVoxels();
	const int32_t height = destRegion.getHeightInVoxels();
	const int32_t width = destRegion.getWidthInVoxels();
	for (int32_t z = 0; z < depth; z++) {
		for (int32_t y = 0; y < height; y++) {
			for (int32_t x = 0; x < width; x++) {
				srcSampler.setPosition(x, y, z);
				const Voxel& voxel = srcSampler.getVoxel();
				if (voxel == emptyVoxel) {
					continue;
				}
				dstSampler.setPosition(x, y, z);
				dstSampler.setVoxel(voxel);
				++cnt;
			}
		}
	}
	return cnt;
}

}
