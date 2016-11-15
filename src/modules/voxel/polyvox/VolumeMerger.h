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
template<typename MergeCondition = MergeConditionSkipVoxelType>
int mergeRawVolumes(RawVolume* destination, const RawVolume* source, const Region& destReg, const Region& sourceReg, MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	int cnt = 0;
	RawVolume::Sampler srcSampler(source);
	RawVolume::Sampler dstSampler(destination);

	core_assert(source->getRegion().containsRegion(sourceReg));
	core_assert(destination->getRegion().containsRegion(destReg));

	for (int32_t z = sourceReg.getLowerZ(); z <= sourceReg.getUpperZ(); ++z) {
		const int destZ = destReg.getLowerZ() + z - sourceReg.getLowerZ();
		for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
			const int destY = destReg.getLowerY() + y - sourceReg.getLowerY();
			for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
				core_assert_always(srcSampler.setPosition(x, y, z));
				const Voxel& voxel = srcSampler.getVoxel();
				if (!mergeCondition(voxel)) {
					continue;
				}
				const int destX = destReg.getLowerX() + x - sourceReg.getLowerX();
				if (dstSampler.setPosition(destX, destY, destZ)) {
					dstSampler.setVoxel(voxel);
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
