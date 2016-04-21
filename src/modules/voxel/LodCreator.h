#pragma once

#include "WorldData.h"
#include "polyvox/RawVolume.h"

namespace voxel {

inline bool isSolidVoxel(const Voxel& voxel) {
	return voxel.getMaterial() != Air;
}

void rescaleCubicVolume(WorldData* source, const PolyVox::Region& sourceRegion, PolyVox::RawVolume<Voxel>* destination, const PolyVox::Region& destinationRegion) {
	core_assert_msg(sourceRegion.getWidthInVoxels() == destinationRegion.getWidthInVoxels() * 2, "Wrong width - %i versus %i!", sourceRegion.getWidthInVoxels(), destinationRegion.getWidthInVoxels() * 2);
	core_assert_msg(sourceRegion.getHeightInVoxels() == destinationRegion.getHeightInVoxels() * 2, "Wrong height - %i versus %i!", sourceRegion.getHeightInVoxels(), destinationRegion.getHeightInVoxels() * 2);
	core_assert_msg(sourceRegion.getDepthInVoxels() == destinationRegion.getDepthInVoxels() * 2, "Wrong depth - %i versus %i!", sourceRegion.getDepthInVoxels(), destinationRegion.getDepthInVoxels() * 2);

	typename WorldData::Sampler srcSampler(source);
	typename PolyVox::RawVolume<Voxel>::Sampler dstSampler(destination);

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

	// At this point the results are usable, but we have a problem with thin structures disappearing.
	// For example, if we have a solid blue sphere with a one voxel thick layer of red voxels on it,
	// then we don't care that the shape changes then the red voxels are lost but we do care that the
	// color changes, as this is very noticeable. Our solution is to process again only those voxels
	// which lie on a material-air boundary, and to recompute their color using a larger neighborhood
	// while also accounting for how visible the child voxels are.
	for (int32_t z = 0; z < destinationRegion.getDepthInVoxels(); z++) {
		for (int32_t y = 0; y < destinationRegion.getHeightInVoxels(); y++) {
			for (int32_t x = 0; x < destinationRegion.getWidthInVoxels(); x++) {
				const glm::ivec3 dstPos = destinationRegion.getLowerCorner() + glm::ivec3(x, y, z);

				dstSampler.setPosition(dstPos);

				// Skip empty voxels
				if (!isSolidVoxel(dstSampler.getVoxel())) {
					continue;
				}
				// Only process voxels on a material-air boundary.
				if (!isSolidVoxel(dstSampler.peekVoxel0px0py1nz())
						|| !isSolidVoxel(dstSampler.peekVoxel0px0py1pz())
						|| !isSolidVoxel(dstSampler.peekVoxel0px1ny0pz())
						|| !isSolidVoxel(dstSampler.peekVoxel0px1py0pz())
						|| !isSolidVoxel(dstSampler.peekVoxel1nx0py0pz())
						|| !isSolidVoxel(dstSampler.peekVoxel1px0py0pz())) {
					const glm::ivec3 srcPos = sourceRegion.getLowerCorner() + (glm::ivec3(x, y, z) * 2);

					uint32_t maxExposedFaces = 0;
					Voxel maxExposedFacesVoxel;

					// Look at the 64 (4x4x4) children
					for (int32_t childZ = -1; childZ < 3; childZ++) {
						for (int32_t childY = -1; childY < 3; childY++) {
							for (int32_t childX = -1; childX < 3; childX++) {
								srcSampler.setPosition(srcPos + glm::ivec3(childX, childY, childZ));

								if (!isSolidVoxel(srcSampler.getVoxel())) {
									continue;
								}
								// For each small voxel, count the exposed faces and use this
								// to determine the importance of the color contribution.
								uint32_t exposedFaces = 0;
								if (!isSolidVoxel(srcSampler.peekVoxel0px0py1nz())) {
									++exposedFaces;
								}
								if (!isSolidVoxel(srcSampler.peekVoxel0px0py1pz())) {
									++exposedFaces;
								}
								if (!isSolidVoxel(srcSampler.peekVoxel0px1ny0pz())) {
									++exposedFaces;
								}
								if (!isSolidVoxel(srcSampler.peekVoxel0px1py0pz())) {
									++exposedFaces;
								}
								if (!isSolidVoxel(srcSampler.peekVoxel1nx0py0pz())) {
									++exposedFaces;
								}
								if (!isSolidVoxel(srcSampler.peekVoxel1px0py0pz())) {
									++exposedFaces;
								}

								if (exposedFaces >= maxExposedFaces) {
									maxExposedFacesVoxel = srcSampler.getVoxel();;
								}
							}
						}
					}

					destination->setVoxel(dstPos, maxExposedFacesVoxel);
				}
			}
		}
	}
}

}
