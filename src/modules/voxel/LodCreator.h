/**
 * @file
 */

#pragma once

#include "polyvox/RawVolume.h"
#include "polyvox/PagedVolume.h"
#include "core/Trace.h"
#include "core/Common.h"

namespace voxel {

void rescaleCubicVolume(PagedVolume* source, const Region& sourceRegion, RawVolume* destination, const Region& destRegion) {
	core_trace_scoped(RescaleCubicVolume);
	core_assert_msg(sourceRegion.getWidthInVoxels() == destRegion.getWidthInVoxels() * 2, "Wrong width - %i versus %i!", sourceRegion.getWidthInVoxels(), destRegion.getWidthInVoxels() * 2);
	core_assert_msg(sourceRegion.getHeightInVoxels() == destRegion.getHeightInVoxels() * 2, "Wrong height - %i versus %i!", sourceRegion.getHeightInVoxels(), destRegion.getHeightInVoxels() * 2);
	core_assert_msg(sourceRegion.getDepthInVoxels() == destRegion.getDepthInVoxels() * 2, "Wrong depth - %i versus %i!", sourceRegion.getDepthInVoxels(), destRegion.getDepthInVoxels() * 2);

	PagedVolume::Sampler srcSampler(source);
	RawVolume::Sampler dstSampler(destination);

	// First of all we iterate over all destination voxels and compute their color as the
	// average of the colors of the eight corresponding voxels in the higher resolution version.
	const int32_t depth = destRegion.getDepthInVoxels();
	const int32_t height = destRegion.getHeightInVoxels();
	const int32_t width = destRegion.getWidthInVoxels();
	for (int32_t z = 0; z < depth; z++) {
		for (int32_t y = 0; y < height; y++) {
			for (int32_t x = 0; x < width; x++) {
				const glm::ivec3& srcPos = sourceRegion.getLowerCorner() + (glm::ivec3(x, y, z) * 2);
				const glm::ivec3& dstPos = destRegion.getLowerCorner() + glm::ivec3(x, y, z);

				uint32_t noOfSolidVoxels = 0;
				for (int32_t childZ = 0; childZ < 2; childZ++) {
					for (int32_t childY = 0; childY < 2; childY++) {
						for (int32_t childX = 0; childX < 2; childX++) {
							srcSampler.setPosition(srcPos.x + childX, srcPos.y + childY, srcPos.z + childZ);
							if (!isBlocked(srcSampler.getVoxel().getMaterial())) {
								continue;
							}
							noOfSolidVoxels++;
						}
					}
				}

				// We only make a voxel solid if the eight corresponding voxels are also all solid. This
				// means that higher LOD meshes actually shrink away which ensures cracks aren't visible.
				if (noOfSolidVoxels > 7) {
					srcSampler.setPosition(srcPos);
					const Voxel& voxel = srcSampler.getVoxel();
					destination->setVoxel(dstPos, voxel);
				} else {
					constexpr Voxel air;
					destination->setVoxel(dstPos, air);
				}
			}
		}
	}
}

}
