/**
 * @file
 */

#pragma once

#include "RawVolume.h"
#include "core/Common.h"
#include "core/Trace.h"

namespace voxel {

class MergeConditionSkipVoxelType {
private:
	voxel::Voxel _voxel;
public:
	MergeConditionSkipVoxelType(voxel::VoxelType type = voxel::VoxelType::Air) :
			_voxel(type) {
	}

	inline bool operator() (const voxel::Voxel& voxel) const {
		return voxel != _voxel;
	}
};

/**
 * @note This version can deal with source volumes that are smaller or equal sized to the destination volume
 */
template<typename MergeCondition = MergeConditionSkipVoxelType, class Volume1, class Volume2>
int mergeRawVolumes(Volume1* destination, const Volume2* source, const Region& destReg, const Region& sourceReg, MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	int cnt = 0;
	for (int32_t z = sourceReg.getLowerZ(); z <= sourceReg.getUpperZ(); ++z) {
		const int destZ = destReg.getLowerZ() + z - sourceReg.getLowerZ();
		for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
			const int destY = destReg.getLowerY() + y - sourceReg.getLowerY();
			for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
				const Voxel& voxel = source->getVoxel(x, y, z);
				if (!mergeCondition(voxel)) {
					continue;
				}
				const int destX = destReg.getLowerX() + x - sourceReg.getLowerX();
				if (destination->setVoxel(destX, destY, destZ, voxel)) {
					++cnt;
				}
			}
		}
	}
	return cnt;
}

template<typename MergeCondition = MergeConditionSkipVoxelType>
inline int mergeRawVolumesSameDimension(RawVolume* destination, const RawVolume* source, MergeCondition mergeCondition = MergeCondition()) {
	core_assert(source->getRegion() == destination->getRegion());
	return mergeRawVolumes(destination, source, destination->getRegion(), source->getRegion());
}

}
