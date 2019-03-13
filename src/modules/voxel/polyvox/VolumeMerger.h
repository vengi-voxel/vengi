/**
 * @file
 */

#pragma once

#include "RawVolume.h"
#include "core/Common.h"
#include "core/Trace.h"
#include <vector>

namespace voxel {

/**
 * @brief Will skip air voxels on volume merges
 */
struct MergeSkipEmpty {
	inline bool operator() (const voxel::Voxel& voxel) const {
		return !isAir(voxel.getMaterial());
	}
};

/**
 * @note This version can deal with source volumes that are smaller or equal sized to the destination volume
 * @note The given merge condition function must return false for voxels that should be skipped.
 * @sa MergeSkipEmpty
 */
template<typename MergeCondition = MergeSkipEmpty, class Volume1, class Volume2>
int mergeVolumes(Volume1* destination, const Volume2* source, const Region& destReg, const Region& sourceReg, MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	int cnt = 0;
	for (int32_t z = sourceReg.getLowerZ(); z <= sourceReg.getUpperZ(); ++z) {
		const int destZ = destReg.getLowerZ() + z - sourceReg.getLowerZ();
		for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
			const int destY = destReg.getLowerY() + y - sourceReg.getLowerY();
			for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
				const Voxel& voxel = source->voxel(x, y, z);
				if (!mergeCondition(voxel)) {
					continue;
				}
				const int destX = destReg.getLowerX() + x - sourceReg.getLowerX();
				if (!destReg.containsPoint(destX, destY, destZ)) {
					continue;
				}
				if (destination->setVoxel(destX, destY, destZ, voxel)) {
					++cnt;
				}
			}
		}
	}
	return cnt;
}

/**
 * The given merge condition function must return false for voxels that should be skipped.
 * @sa MergeSkipEmpty
 */
template<typename MergeCondition = MergeSkipEmpty>
inline int mergeRawVolumesSameDimension(RawVolume* destination, const RawVolume* source, MergeCondition mergeCondition = MergeCondition()) {
	core_assert(source->region() == destination->region());
	return mergeVolumes(destination, source, destination->region(), source->region());
}

extern RawVolume* merge(const std::vector<RawVolume*>& volumes);

}
