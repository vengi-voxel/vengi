/**
 * @file
 */
#pragma once

#include "app/App.h"
#include "app/Async.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "palette/Palette.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxelutil {

/**
 * @brief Rescales a volume by sampling two voxels to produce one output voxel.
 * @param[in] sourceVolume The source volume to resample
 * @param[in] destVolume The destination volume to resample into
 * @param[in] sourceRegion The region of the source volume to resample
 * @param[in] destRegion The region of the destination volume to resample into. Usually this should
 * be exactly half of the size of the sourceRegion.
 */
template<typename SourceVolume, typename DestVolume>
void scaleDown(const SourceVolume &sourceVolume, const palette::Palette &palette, const voxel::Region &sourceRegion,
			   DestVolume &destVolume, const voxel::Region &destRegion) {
	core_trace_scoped(ScaleVolumeDown);

	const int32_t depth = destRegion.getDepthInVoxels();
	// First of all we iterate over all destination voxels and compute their color as the
	// avg of the colors of the eight corresponding voxels in the higher resolution version.
	app::for_parallel(0, depth, [&sourceVolume, sourceRegion, &palette, &destVolume, destRegion](int start, int end) {
		const int32_t height = destRegion.getHeightInVoxels();
		const int32_t width = destRegion.getWidthInVoxels();
		for (int32_t z = start; z < end; ++z) {
			for (int32_t y = 0; y < height; ++y) {
				for (int32_t x = 0; x < width; ++x) {
					const glm::ivec3 curPos(x, y, z);
					const glm::ivec3 srcPos = sourceRegion.getLowerCorner() + curPos * 2;
					const glm::ivec3 dstPos = destRegion.getLowerCorner() + curPos;

					float colorContributors = 0.0f;
					float solidVoxels = 0.0f;
					float avgColorRed = 0.0f;
					float avgColorGreen = 0.0f;
					float avgColorBlue = 0.0f;
					voxel::Voxel colorGuardVoxel;

					typename SourceVolume::Sampler srcSampler1(sourceVolume);
					srcSampler1.setPosition(srcPos);
					for (int32_t childZ = 0; childZ < 2; ++childZ) {
						typename SourceVolume::Sampler srcSampler2 = srcSampler1;
						for (int32_t childY = 0; childY < 2; ++childY) {
							typename SourceVolume::Sampler srcSampler3 = srcSampler2;
							for (int32_t childX = 0; childX < 2; ++childX) {
								if (!srcSampler3.currentPositionValid()) {
									srcSampler3.movePositiveX();
									continue;
								}
								const voxel::Voxel &child = srcSampler3.voxel();

								if (isBlocked(child.getMaterial())) {
									++solidVoxels;
									if (voxel::FaceBits::None == voxel::visibleFaces(srcSampler3)) {
										colorGuardVoxel = child;
										srcSampler3.movePositiveX();
										continue;
									}
									const glm::vec4 &color = color::fromRGBA(palette.color(child.getColor()));
									avgColorRed += color.r;
									avgColorGreen += color.g;
									avgColorBlue += color.b;
									++colorContributors;
								}
								srcSampler3.movePositiveX();
							}
							srcSampler2.movePositiveY();
						}
						srcSampler1.movePositiveZ();
					}

					// We only make a voxel solid if the eight corresponding voxels are also all solid. This
					// means that higher LOD meshes actually shrink away which ensures cracks aren't visible.
					if (solidVoxels >= 7.0f) {
						if (colorContributors <= 0.0f) {
							const glm::vec4 &color = color::fromRGBA(palette.color(colorGuardVoxel.getColor()));
							avgColorRed += color.r;
							avgColorGreen += color.g;
							avgColorBlue += color.b;
							++colorContributors;
						}
						const glm::vec4 avgColor(avgColorRed / colorContributors, avgColorGreen / colorContributors,
												avgColorBlue / colorContributors, 1.0f);
						color::RGBA avgRGBA = color::getRGBA(avgColor);
						const int index = palette.getClosestMatch(avgRGBA);
						voxel::Voxel voxel = voxel::createVoxel(palette, index);
						destVolume.setVoxel(dstPos, voxel);
					} else {
						const voxel::Voxel voxelAir;
						destVolume.setVoxel(dstPos, voxelAir);
					}
				}
			}
		}
	});

	// At this point the results are usable, but we have a problem with thin structures disappearing.
	// For example, if we have a solid blue sphere with a one voxel thick layer of red voxels on it,
	// then we don't care that the shape changes then the red voxels are lost but we do care that the
	// color changes, as this is very noticeable. Our solution is to process again only those voxels
	// which lie on a material-air boundary, and to recompute their color using a larger neighborhood
	// while also accounting for how visible the child voxels are.
	app::for_parallel(0, depth, [&sourceVolume, sourceRegion, &palette, &destVolume, destRegion](int start, int end) {
		typename DestVolume::Sampler dstSampler1(destVolume);
		glm::ivec3 pos = destRegion.getLowerCorner();
		pos.z += start;
		dstSampler1.setPosition(pos);
		for (int32_t z = start; z < end; ++z) {
			typename DestVolume::Sampler dstSampler2 = dstSampler1;
			for (int32_t y = 0; y < destRegion.getHeightInVoxels(); ++y) {
				typename DestVolume::Sampler dstSampler3 = dstSampler2;
				for (int32_t x = 0; x < destRegion.getWidthInVoxels(); ++x) {
					// Skip empty voxels
					if (dstSampler3.voxel().getMaterial() == voxel::VoxelType::Air) {
						dstSampler3.movePositiveX();
						continue;
					}
					// Only process voxels on a material-air boundary.
					if (dstSampler3.peekVoxel0px0py1nz().getMaterial() != voxel::VoxelType::Air &&
						dstSampler3.peekVoxel0px0py1pz().getMaterial() != voxel::VoxelType::Air &&
						dstSampler3.peekVoxel0px1ny0pz().getMaterial() != voxel::VoxelType::Air &&
						dstSampler3.peekVoxel0px1py0pz().getMaterial() != voxel::VoxelType::Air &&
						dstSampler3.peekVoxel1nx0py0pz().getMaterial() != voxel::VoxelType::Air &&
						dstSampler3.peekVoxel1px0py0pz().getMaterial() != voxel::VoxelType::Air) {
						dstSampler3.movePositiveX();
						continue;
					}
					const glm::ivec3 srcPos = sourceRegion.getLowerCorner() + dstSampler3.position() * 2;

					float totalRed = 0.0f;
					float totalGreen = 0.0f;
					float totalBlue = 0.0f;
					float totalExposedFaces = 0.0f;

					typename SourceVolume::Sampler srcSampler1(sourceVolume);
					srcSampler1.setPosition(srcPos - 1);
					// Look at the 64 (4x4x4) children
					for (int32_t childZ = -1; childZ < 3; childZ++) {
						typename SourceVolume::Sampler srcSampler2 = srcSampler1;
						for (int32_t childY = -1; childY < 3; childY++) {
							typename SourceVolume::Sampler srcSampler3 = srcSampler2;
							for (int32_t childX = -1; childX < 3; childX++) {
								const voxel::Voxel &child = srcSampler3.voxel();
								if (child.getMaterial() == voxel::VoxelType::Air) {
									srcSampler3.movePositiveX();
									continue;
								}

								// For each small voxel, count the exposed faces and use this
								// to determine the importance of the color contribution.
								float exposedFaces = 0.0f;
								if (srcSampler3.peekVoxel0px0py1nz().getMaterial() == voxel::VoxelType::Air) {
									++exposedFaces;
								}
								if (srcSampler3.peekVoxel0px0py1pz().getMaterial() == voxel::VoxelType::Air) {
									++exposedFaces;
								}
								if (srcSampler3.peekVoxel0px1ny0pz().getMaterial() == voxel::VoxelType::Air) {
									++exposedFaces;
								}
								if (srcSampler3.peekVoxel0px1py0pz().getMaterial() == voxel::VoxelType::Air) {
									++exposedFaces;
								}
								if (srcSampler3.peekVoxel1nx0py0pz().getMaterial() == voxel::VoxelType::Air) {
									++exposedFaces;
								}
								if (srcSampler3.peekVoxel1px0py0pz().getMaterial() == voxel::VoxelType::Air) {
									++exposedFaces;
								}

								const glm::vec4 &color = color::fromRGBA(palette.color(child.getColor()));
								totalRed += color.r * exposedFaces;
								totalGreen += color.g * exposedFaces;
								totalBlue += color.b * exposedFaces;

								totalExposedFaces += exposedFaces;
								srcSampler3.movePositiveX();
							}
							srcSampler2.movePositiveY();
						}
						srcSampler1.movePositiveZ();
					}

					// Avoid divide by zero if there were no exposed faces.
					if (totalExposedFaces <= 0.01f) {
						++totalExposedFaces;
					}

					const glm::vec4 avgColor(totalRed / totalExposedFaces, totalGreen / totalExposedFaces,
											totalBlue / totalExposedFaces, 1.0f);
					color::RGBA avgRGBA = color::getRGBA(avgColor);
					const int index = palette.getClosestMatch(avgRGBA);
					const voxel::Voxel voxel = voxel::createVoxel(palette, index);
					dstSampler3.setVoxel(voxel);
					dstSampler3.movePositiveX();
				}
				dstSampler2.movePositiveY();
			}
			dstSampler1.movePositiveZ();
		}
	});
}

template<typename SourceVolume, typename DestVolume>
void scaleDown(const SourceVolume &sourceVolume, const palette::Palette &palette, DestVolume &destVolume) {
	scaleDown(sourceVolume, palette, sourceVolume.region(), destVolume, destVolume.region());
}

[[nodiscard]] voxel::RawVolume *scaleUp(const voxel::RawVolume &sourceVolume);

} // namespace voxelutil
