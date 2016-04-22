#pragma once

#include "polyvox/RawVolume.h"
#include "WorldData.h"

namespace voxel {

inline bool isSolidVoxel(const Voxel& voxel) {
	return voxel.getMaterial() != Air;
}

void rescaleCubicVolume(PagedVolume* source, const Region& sourceRegion, RawVolume<Voxel>* destination, const Region& destinationRegion) {
	core_assert_msg(sourceRegion.getWidthInVoxels() == destinationRegion.getWidthInVoxels() * 2, "Wrong width - %i versus %i!", sourceRegion.getWidthInVoxels(), destinationRegion.getWidthInVoxels() * 2);
	core_assert_msg(sourceRegion.getHeightInVoxels() == destinationRegion.getHeightInVoxels() * 2, "Wrong height - %i versus %i!", sourceRegion.getHeightInVoxels(), destinationRegion.getHeightInVoxels() * 2);
	core_assert_msg(sourceRegion.getDepthInVoxels() == destinationRegion.getDepthInVoxels() * 2, "Wrong depth - %i versus %i!", sourceRegion.getDepthInVoxels(), destinationRegion.getDepthInVoxels() * 2);

	typename PagedVolume::Sampler srcSampler(source);
	typename RawVolume<Voxel>::Sampler dstSampler(destination);

	// First of all we iterate over all destination voxels and compute their color as the
	// average of the colors of the eight corresponding voxels in the higher resolution version.
	for (int32_t z = 0; z < destinationRegion.getDepthInVoxels(); z++) {
		for (int32_t y = 0; y < destinationRegion.getHeightInVoxels(); y++) {
			for (int32_t x = 0; x < destinationRegion.getWidthInVoxels(); x++) {
				const glm::ivec3 srcPos = sourceRegion.getLowerCorner() + (glm::ivec3(x, y, z) * 2);
				const glm::ivec3 dstPos = destinationRegion.getLowerCorner() + glm::ivec3(x, y, z);

				uint32_t noOfSolidVoxels = 0;
				for (int32_t childZ = 0; childZ < 2; childZ++) {
					for (int32_t childY = 0; childY < 2; childY++) {
						for (int32_t childX = 0; childX < 2; childX++) {
							srcSampler.setPosition(srcPos.x + childX, srcPos.y + childY, srcPos.z + childZ);
							if (!isSolidVoxel(srcSampler.getVoxel())) {
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
					Voxel voxel = createVoxel(Air);
					destination->setVoxel(dstPos, voxel);
				}
			}
		}
	}
}

}
