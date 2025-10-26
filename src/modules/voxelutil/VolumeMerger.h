/**
 * @file
 */

#pragma once

#include "app/Async.h"
#include "core/collection/Buffer.h"
#include "core/concurrent/Atomic.h"
#include "palette/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "palette/Palette.h"
#include "core/Trace.h"
#include "core/Assert.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

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
	app::for_parallel(sourceReg.getLowerZ(), sourceReg.getUpperZ() + 1, [&source, &destination, sourceReg, destReg, &cnt, &mergeCondition] (int start, int end)	{
		typename Volume2::Sampler sourceSampler(source);
		typename Volume1::Sampler destSampler(destination);
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
						++cnt;
					}
					destSampler.movePositiveX();
				}
				sourceSampler2.movePositiveY();
			}
			sourceSampler.movePositiveZ();
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
	palette::PaletteLookup palLookup(destinationPalette);
	app::for_parallel(sourceReg.getLowerZ(), sourceReg.getUpperZ() + 1, [&source, &destination, sourceReg, destReg, &cnt, &mergeCondition, &sourcePalette, &palLookup, &destinationPalette] (int start, int end)	{
		typename Volume2::Sampler sourceSampler(source);
		typename Volume1::Sampler destSampler(destination);
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
					int idx = palLookup.findClosestIndex(sourcePalette.color(srcVoxel.getColor()));
					if (idx == palette::PaletteColorNotFound) {
						idx = 0;
					}
					const voxel::Voxel destVoxel = voxel::createVoxel(destinationPalette, idx);
					if (destSampler.setVoxel(destVoxel)) {
						++cnt;
					}
					destSampler.movePositiveX();
				}
				sourceSampler2.movePositiveY();
			}
			sourceSampler.movePositiveZ();
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

}
