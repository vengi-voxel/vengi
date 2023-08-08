/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxel/RawVolume.h"
#include "core/Trace.h"
#include "core/Assert.h"

namespace voxelutil {

/**
 * @brief Will skip air voxels on volume merges
 */
struct MergeSkipEmpty {
	inline bool operator() (voxel::Voxel& voxel) const {
		return !isAir(voxel.getMaterial());
	}
};

/**
 * @note This version can deal with source volumes that are smaller or equal sized to the destination volume
 * @note The given merge condition function must return false for voxels that should be skipped.
 * @sa MergeSkipEmpty
 */
template<typename MergeCondition = MergeSkipEmpty, class Volume1, class Volume2>
int mergeVolumes(Volume1* destination, const Volume2* source, const voxel::Region& destReg, const voxel::Region& sourceReg, MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	int cnt = 0;
	typename Volume2::Sampler sourceSampler(source);
	typename Volume1::Sampler destSampler(destination);
	const int relX = destReg.getLowerX();
	sourceSampler.setPosition(sourceReg.getLowerCorner());
	for (int32_t z = sourceReg.getLowerZ(); z <= sourceReg.getUpperZ(); ++z) {
		const int destZ = destReg.getLowerZ() + z - sourceReg.getLowerZ();
		typename Volume2::Sampler sourceSampler2 = sourceSampler;
		for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
			const int destY = destReg.getLowerY() + y - sourceReg.getLowerY();
			typename Volume2::Sampler sourceSampler3 = sourceSampler2;
			destSampler.setPosition(relX, destY, destZ);
			for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
				voxel::Voxel voxel = sourceSampler3.voxel();
				sourceSampler3.movePositiveX();
				if (!mergeCondition(voxel)) {
					destSampler.movePositiveX();
					continue;
				}
				if (destSampler.setVoxel(voxel)) {
					++cnt;
				}
				destSampler.movePositiveX();
			}
			sourceSampler2.movePositiveY();
		}
		sourceSampler.movePositiveZ();
	}
	return cnt;
}

/**
 * The given merge condition function must return false for voxels that should be skipped.
 * @sa MergeSkipEmpty
 */
template<typename MergeCondition = MergeSkipEmpty>
inline int mergeRawVolumesSameDimension(voxel::RawVolume* destination, const voxel::RawVolume* source, MergeCondition mergeCondition = MergeCondition()) {
	core_assert(source->region() == destination->region());
	return mergeVolumes(destination, source, destination->region(), source->region());
}

extern voxel::RawVolume* merge(const core::DynamicArray<voxel::RawVolume*>& volumes);
extern voxel::RawVolume* merge(const core::DynamicArray<const voxel::RawVolume*>& volumes);

}
