/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "core/TraceScoped.h"
#include "core/collection/Buffer.h"
#include "core/concurrent/Atomic.h"
#include "palette/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxel/VolumeSamplerDispatch.h"
#include "voxel/VolumeTileDispatch.h"
#include "palette/Palette.h"
#include "core/Assert.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <type_traits>

namespace voxelutil {

/**
 * @note This version can deal with source volumes that are smaller or equal sized to the destination volume
 * @note The given merge condition function must return false for voxels that should be skipped.
 * @sa voxelutil::VisitSolid
 */
template<typename MergeCondition = VisitSolid, class Volume1, class Volume2>
int mergeVolumes(Volume1 *destination, const Volume2 *source, const voxel::Region &destReg,
				 const voxel::Region &sourceReg, MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	core::AtomicInt cnt = 0;
	if constexpr (std::is_same_v<Volume1, voxel::RawVolume>) {
		if (destination->isSparse()) {
			const glm::ivec3 offset = destReg.getLowerCorner() - sourceReg.getLowerCorner();
			const voxel::Region targetReg(destReg.getLowerCorner(),
										  destReg.getLowerCorner() + sourceReg.getDimensionsInVoxels() - 1);
			voxel::forEachTileWriteParallel(*destination, targetReg, [&](voxel::ParallelTileWriter &tile) {
				const voxel::Region &tr = tile.region();
				const voxel::Region srcTile(tr.getLowerCorner() - offset, tr.getUpperCorner() - offset);
				if constexpr (std::is_same_v<Volume2, voxel::RawVolume> && ConditionSkipsAir<MergeCondition>::value) {
					if (source->isSparseAirDefault() && !source->hasOccupiedVoxelsInRegion(srcTile)) {
						return;
					}
				}
				int localCnt = 0;
				voxel::withVolumeSampler(*source, [&](auto &srcSampler) {
					srcSampler.setPosition(srcTile.getLowerCorner());
					for (int32_t z = srcTile.getLowerZ(); z <= srcTile.getUpperZ(); ++z) {
						const int32_t dz = z + offset.z;
						auto srcSamplerY = srcSampler;
						for (int32_t y = srcTile.getLowerY(); y <= srcTile.getUpperY(); ++y) {
							const int32_t dy = y + offset.y;
							auto srcSamplerX = srcSamplerY;
							for (int32_t x = srcTile.getLowerX(); x <= srcTile.getUpperX(); ++x) {
								if (mergeCondition(srcSamplerX)) {
									if (tile.setVoxel(x + offset.x, dy, dz, srcSamplerX.voxel())) {
										++localCnt;
									}
								}
								srcSamplerX.movePositiveX();
							}
							srcSamplerY.movePositiveY();
						}
						srcSampler.movePositiveZ();
					}
				});
				if (localCnt > 0) {
					cnt.increment(localCnt);
				}
			});
			return cnt;
		}
	}
	app::for_parallel(sourceReg.getLowerZ(), sourceReg.getUpperZ() + 1, [&source, &destination, sourceReg, destReg, &cnt, &mergeCondition] (int start, int end)	{
		typename Volume2::Sampler sourceSampler(source);
		typename Volume1::Sampler destSampler(destination);
		int localCnt = 0;
		const int relX = destReg.getLowerX();
		sourceSampler.setPosition(sourceReg.getLowerX(), sourceReg.getLowerY(), start);
		for (int32_t z = start; z < end; ++z) {
			const int destZ = destReg.getLowerZ() + z - sourceReg.getLowerZ();
			typename Volume2::Sampler sourceSampler2 = sourceSampler;
			for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
				const int destY = destReg.getLowerY() + y - sourceReg.getLowerY();
				typename Volume2::Sampler sourceSampler3 = sourceSampler2;
				destSampler.setPosition(relX, destY, destZ);
				for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
					voxel::Voxel srcVoxel = sourceSampler3.voxel();
					if (!mergeCondition(sourceSampler3)) {
						sourceSampler3.movePositiveX();
						destSampler.movePositiveX();
						continue;
					}
					sourceSampler3.movePositiveX();
					if (destSampler.setVoxel(srcVoxel)) {
						++localCnt;
					}
					destSampler.movePositiveX();
				}
				sourceSampler2.movePositiveY();
			}
			sourceSampler.movePositiveZ();
		}
		if (localCnt > 0) {
			cnt.increment(localCnt);
		}
	});
	return cnt;
}

/**
 * @note This version can deal with source volumes that are smaller or equal sized to the destination volume
 * @note The given merge condition function must return false for voxels that should be skipped.
 * @sa voxelutil::VisitSolid
 */
template<typename MergeCondition = VisitSolid, class Volume1, class Volume2>
int mergeVolumes(Volume1 *destination, const palette::Palette &destinationPalette, const Volume2 *source,
				 const palette::Palette &sourcePalette, const voxel::Region &destReg, const voxel::Region &sourceReg,
				 MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	core::AtomicInt cnt = 0;
	voxel::Voxel remapVoxel[palette::PaletteMaxColors];
	{
		palette::PaletteLookup palLookup(destinationPalette);
		for (int c = 0; c < palette::PaletteMaxColors; ++c) {
			int idx = palLookup.findClosestIndex(sourcePalette.color((uint8_t)c));
			if (idx == palette::PaletteColorNotFound) {
				idx = 0;
			}
			remapVoxel[c] = voxel::createVoxel(destinationPalette, idx);
		}
	}
	if constexpr (std::is_same_v<Volume1, voxel::RawVolume>) {
		if (destination->isSparse()) {
			const glm::ivec3 offset = destReg.getLowerCorner() - sourceReg.getLowerCorner();
			const voxel::Region targetReg(destReg.getLowerCorner(),
										  destReg.getLowerCorner() + sourceReg.getDimensionsInVoxels() - 1);
			voxel::forEachTileWriteParallel(*destination, targetReg, [&](voxel::ParallelTileWriter &tile) {
				const voxel::Region &tr = tile.region();
				const voxel::Region srcTile(tr.getLowerCorner() - offset, tr.getUpperCorner() - offset);
				if constexpr (std::is_same_v<Volume2, voxel::RawVolume> && ConditionSkipsAir<MergeCondition>::value) {
					if (source->isSparseAirDefault() && !source->hasOccupiedVoxelsInRegion(srcTile)) {
						return;
					}
				}
				int localCnt = 0;
				voxel::withVolumeSampler(*source, [&](auto &srcSampler) {
					srcSampler.setPosition(srcTile.getLowerCorner());
					for (int32_t z = srcTile.getLowerZ(); z <= srcTile.getUpperZ(); ++z) {
						const int32_t dz = z + offset.z;
						auto srcSamplerY = srcSampler;
						for (int32_t y = srcTile.getLowerY(); y <= srcTile.getUpperY(); ++y) {
							const int32_t dy = y + offset.y;
							auto srcSamplerX = srcSamplerY;
							for (int32_t x = srcTile.getLowerX(); x <= srcTile.getUpperX(); ++x) {
								if (mergeCondition(srcSamplerX)) {
									if (tile.setVoxel(x + offset.x, dy, dz, remapVoxel[srcSamplerX.voxel().getColor()])) {
										++localCnt;
									}
								}
								srcSamplerX.movePositiveX();
							}
							srcSamplerY.movePositiveY();
						}
						srcSampler.movePositiveZ();
					}
				});
				if (localCnt > 0) {
					cnt.increment(localCnt);
				}
			});
			return cnt;
		}
	}
	app::for_parallel(sourceReg.getLowerZ(), sourceReg.getUpperZ() + 1, [&source, &destination, sourceReg, destReg, &cnt, &mergeCondition, &remapVoxel] (int start, int end)	{
		typename Volume2::Sampler sourceSampler(source);
		typename Volume1::Sampler destSampler(destination);
		int localCnt = 0;
		const int relX = destReg.getLowerX();
		sourceSampler.setPosition(sourceReg.getLowerX(), sourceReg.getLowerY(), start);
		for (int32_t z = start; z < end; ++z) {
			const int destZ = destReg.getLowerZ() + z - sourceReg.getLowerZ();
			typename Volume2::Sampler sourceSampler2 = sourceSampler;
			for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
				const int destY = destReg.getLowerY() + y - sourceReg.getLowerY();
				typename Volume2::Sampler sourceSampler3 = sourceSampler2;
				destSampler.setPosition(relX, destY, destZ);
				for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
					voxel::Voxel srcVoxel = sourceSampler3.voxel();
					if (!mergeCondition(sourceSampler3)) {
						sourceSampler3.movePositiveX();
						destSampler.movePositiveX();
						continue;
					}
					sourceSampler3.movePositiveX();
					if (destSampler.setVoxel(remapVoxel[srcVoxel.getColor()])) {
						++localCnt;
					}
					destSampler.movePositiveX();
				}
				sourceSampler2.movePositiveY();
			}
			sourceSampler.movePositiveZ();
		}
		if (localCnt > 0) {
			cnt.increment(localCnt);
		}
	});
	return cnt;
}

/**
 * The given merge condition function must return false for voxels that should be skipped.
 * @sa voxelutil::VisitSolid
 */
template<typename MergeCondition = VisitSolid>
inline int mergeRawVolumesSameDimension(voxel::RawVolume* destination, const voxel::RawVolume* source, MergeCondition mergeCondition = MergeCondition()) {
	core_assert(source->region() == destination->region());
	return mergeVolumes(destination, source, destination->region(), source->region());
}

[[nodiscard]] voxel::RawVolume* merge(const core::Buffer<voxel::RawVolume*>& volumes);
[[nodiscard]] voxel::RawVolume* merge(const core::Buffer<const voxel::RawVolume*>& volumes);

/**
 * @brief Merge @p volumes into one volume sized to the union of their solid-voxel bounds.
 * @return merged volume or @c nullptr if no solid voxels were found. Caller owns the result.
 */
[[nodiscard]] voxel::RawVolume *mergeAndCrop(const core::Buffer<voxel::RawVolume *> &volumes);
[[nodiscard]] voxel::RawVolume *mergeAndCrop(const core::Buffer<const voxel::RawVolume *> &volumes);

}
