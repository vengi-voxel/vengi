/**
 * @file
 */

#pragma once

#include "RawVolume.h"

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

template<typename MergeCondition = MergeConditionSkipVoxelType>
int mergeRawVolumesSameDimension(RawVolume* destination, const RawVolume* source, MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	int cnt = 0;
	RawVolume::Sampler srcSampler(source);
	RawVolume::Sampler dstSampler(destination);
	const Region& destRegion = destination->getEnclosingRegion();
	core_assert_always(destRegion == source->getEnclosingRegion());
	const int32_t depth = destRegion.getDepthInVoxels();
	const int32_t height = destRegion.getHeightInVoxels();
	const int32_t width = destRegion.getWidthInVoxels();
	for (int32_t z = 0; z < depth; z++) {
		for (int32_t y = 0; y < height; y++) {
			for (int32_t x = 0; x < width; x++) {
				srcSampler.setPosition(x, y, z);
				const Voxel& voxel = srcSampler.getVoxel();
				if (!mergeCondition(voxel)) {
					continue;
				}
				dstSampler.setPosition(x, y, z);
				dstSampler.setVoxel(voxel);
				++cnt;
			}
		}
	}
	return cnt;
}

/**
 * @note This version can deal with source volumes that are smaller or equal sized to the destination volume
 */
template<typename MergeCondition = MergeConditionSkipVoxelType>
int mergeRawVolumes(RawVolume* destination, const RawVolume* source, const glm::ivec3& sourceOffset, MergeCondition mergeCondition = MergeCondition()) {
	core_trace_scoped(MergeRawVolumes);
	int cnt = 0;
	RawVolume::Sampler srcSampler(source);
	RawVolume::Sampler dstSampler(destination);
	const Region& destRegion = destination->getEnclosingRegion();
	const Region& srcRegion = source->getEnclosingRegion();
	const int32_t depth = glm::min(srcRegion.getDepthInVoxels(), destRegion.getDepthInVoxels());
	const int32_t height = glm::min(srcRegion.getHeightInVoxels(), destRegion.getHeightInVoxels());
	const int32_t width = glm::min(srcRegion.getWidthInVoxels(), destRegion.getWidthInVoxels());
	for (int32_t z = sourceOffset.z; z < sourceOffset.z + depth; z++) {
		for (int32_t y = sourceOffset.y; y < sourceOffset.y + height; y++) {
			for (int32_t x = sourceOffset.x; x < sourceOffset.x + width; x++) {
				srcSampler.setPosition(x - sourceOffset.x, y - sourceOffset.y, z - sourceOffset.z);
				const Voxel& voxel = srcSampler.getVoxel();
				if (!mergeCondition(voxel)) {
					continue;
				}
				dstSampler.setPosition(x, y, z);
				dstSampler.setVoxel(voxel);
				++cnt;
			}
		}
	}
	return cnt;
}

}
