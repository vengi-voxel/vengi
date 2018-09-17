#pragma once

#include "core/Common.h"
#include "core/Color.h"
#include "voxel/MaterialColor.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/polyvox/Region.h"

namespace voxel {

/**
 * @brief Rescales a volume by sampling two voxels to produce one output voxel.
 * @param[in] sourceVolume The source volume to resample
 * @param[in] destVolume The destination volume to resample into
 * @param[in] sourceRegion The region of the source volume to resample
 * @param[in] destRegion The region of the destination volume to resample into. Usually this should
 * be exactly half of the size of the sourceRegion.
 */
template<typename SourceVolume, typename DestVolume>
void rescaleVolume(const SourceVolume& sourceVolume, const Region& sourceRegion, DestVolume& destVolume, const Region& destRegion) {
	core_trace_scoped(RescaleVolume);
	typename SourceVolume::Sampler srcSampler(sourceVolume);

	const MaterialColorArray& colors = getMaterialColors();

	const int32_t depth = destRegion.getDepthInVoxels();
	const int32_t height = destRegion.getHeightInVoxels();
	const int32_t width = destRegion.getWidthInVoxels();
	// First of all we iterate over all destination voxels and compute their color as the
	// avg of the colors of the eight corresponding voxels in the higher resolution version.
	for (int32_t z = 0; z < depth; ++z) {
		for (int32_t y = 0; y < height; ++y) {
			for (int32_t x = 0; x < width; ++x) {
				const glm::ivec3 curPos(x, y, z);
				const glm::ivec3 srcPos = sourceRegion.getLowerCorner() + curPos * 2;
				const glm::ivec3 dstPos = destRegion.getLowerCorner() + curPos;

				float solidVoxels = 0.0f;
				float avgOf8Red = 0.0f;
				float avgOf8Green = 0.0f;
				float avgOf8Blue = 0.0f;
				for (int32_t childZ = 0; childZ < 2; ++childZ) {
					for (int32_t childY = 0; childY < 2; ++childY) {
						for (int32_t childX = 0; childX < 2; ++childX) {
							srcSampler.setPosition(srcPos + glm::ivec3(childX, childY, childZ));
							const Voxel& child = srcSampler.voxel();

							if (isBlocked(child.getMaterial())) {
								++solidVoxels;
								const glm::vec4& color = colors[child.getColor()];
								avgOf8Red += color.r;
								avgOf8Green += color.g;
								avgOf8Blue += color.b;
							}
						}
					}
				}

				// We only make a voxel solid if the eight corresponding voxels are also all solid. This
				// means that higher LOD meshes actually shrink away which ensures cracks aren't visible.
				if (solidVoxels >= 7.0f) {
					const glm::vec4 avgColor(avgOf8Red / solidVoxels, avgOf8Green / solidVoxels, avgOf8Blue / solidVoxels, 1.0f);
					const int index = core::Color::getClosestMatch(avgColor, colors);
					Voxel voxel = createVoxel(VoxelType::Generic, index);
					destVolume.setVoxel(dstPos, voxel);
				} else {
					const Voxel voxelAir;
					destVolume.setVoxel(dstPos, voxelAir);
				}
			}
		}
	}

	// At this point the results are usable, but we have a problem with thin structures disappearing.
	// For example, if we have a solid blue sphere with a one voxel thick layer of red voxels on it,
	// then we don't care that the shape changes then the red voxels are lost but we do care that the
	// color changes, as this is very noticable. Our solution is to process again only those voxels
	// which lie on a material-air boundary, and to recompute their color using a larger naighbourhood
	// while also accounting for how visible the child voxels are.
	typename DestVolume::Sampler dstSampler(destVolume);
	for (int32_t z = 0; z < destRegion.getDepthInVoxels(); ++z) {
		for (int32_t y = 0; y < destRegion.getHeightInVoxels(); ++y) {
			for (int32_t x = 0; x < destRegion.getWidthInVoxels(); ++x) {
				const glm::ivec3 curPos(x, y, z);
				const glm::ivec3 dstPos = destRegion.getLowerCorner() + curPos;

				dstSampler.setPosition(dstPos);

				// Skip empty voxels
				if (dstSampler.voxel().getMaterial() == VoxelType::Air) {
					continue;
				}
				// Only process voxels on a material-air boundary.
				if (dstSampler.peekVoxel0px0py1nz().getMaterial() != VoxelType::Air && dstSampler.peekVoxel0px0py1pz().getMaterial() != VoxelType::Air
						&& dstSampler.peekVoxel0px1ny0pz().getMaterial() != VoxelType::Air && dstSampler.peekVoxel0px1py0pz().getMaterial() != VoxelType::Air
						&& dstSampler.peekVoxel1nx0py0pz().getMaterial() != VoxelType::Air && dstSampler.peekVoxel1px0py0pz().getMaterial() != VoxelType::Air) {
					continue;
				}
				const glm::ivec3 srcPos = sourceRegion.getLowerCorner() + curPos * 2;

				float totalRed = 0.0f;
				float totalGreen = 0.0f;
				float totalBlue = 0.0f;
				float totalExposedFaces = 0.0f;

				// Look at the 64 (4x4x4) children
				for (int32_t childZ = -1; childZ < 3; childZ++) {
					for (int32_t childY = -1; childY < 3; childY++) {
						for (int32_t childX = -1; childX < 3; childX++) {
							srcSampler.setPosition(srcPos + glm::ivec3(childX, childY, childZ));

							const Voxel& child = srcSampler.voxel();
							if (child.getMaterial() == VoxelType::Air) {
								continue;
							}

							// For each small voxel, count the exposed faces and use this
							// to determine the importance of the color contribution.
							float exposedFaces = 0.0f;
							if (srcSampler.peekVoxel0px0py1nz().getMaterial() == VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel0px0py1pz().getMaterial() == VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel0px1ny0pz().getMaterial() == VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel0px1py0pz().getMaterial() == VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel1nx0py0pz().getMaterial() == VoxelType::Air) {
								++exposedFaces;
							}
							if (srcSampler.peekVoxel1px0py0pz().getMaterial() == VoxelType::Air) {
								++exposedFaces;
							}

							const glm::vec4& color = colors[child.getColor()];
							totalRed += color.r * exposedFaces;
							totalGreen += color.g * exposedFaces;
							totalBlue += color.b * exposedFaces;

							totalExposedFaces += exposedFaces;
						}
					}
				}

				// Avoid divide by zero if there were no exposed faces.
				if (totalExposedFaces <= 0.01f) {
					++totalExposedFaces;
				}

				const glm::vec4 avgColor(totalRed / totalExposedFaces, totalGreen / totalExposedFaces, totalBlue / totalExposedFaces, 1.0f);
				const int index = core::Color::getClosestMatch(avgColor, colors);
				const Voxel voxel = createVoxel(VoxelType::Generic, index);
				destVolume.setVoxel(dstPos, voxel);
			}
		}
	}
}

template<typename SourceVolume, typename DestVolume>
void rescaleVolume(const SourceVolume& sourceVolume, DestVolume& destVolume) {
	rescaleVolume(sourceVolume, sourceVolume.region(), destVolume, destVolume.region());
}

}
