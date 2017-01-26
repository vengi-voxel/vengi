#pragma once

#include "RawVolume.h"
#include "VolumeMerger.h"

namespace voxel {

/**
 * @brief Will skip air voxels on volume cropping
 */
struct CropSkipEmpty {
	inline bool operator() (const voxel::Voxel& voxel) const {
		return isAir(voxel.getMaterial());
	}
};

template<class CropSkipCondition = CropSkipEmpty>
RawVolume* cropVolume(const RawVolume* volume, const glm::ivec3& mins, const glm::ivec3& maxs, CropSkipCondition condition = CropSkipCondition()) {
	core_trace_scoped(CropRawVolume);
	const voxel::Region newRegion(glm::ivec3(), maxs - mins);
	if (!newRegion.isValid()) {
		return nullptr;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(newRegion);
	voxel::mergeRawVolumes(newVolume, volume, newRegion, voxel::Region(mins, maxs));
	return newVolume;
}

template<class CropSkipCondition = CropSkipEmpty>
RawVolume* cropVolume(const RawVolume* volume, CropSkipCondition condition = CropSkipCondition()) {
	core_trace_scoped(CropRawVolume);
	const voxel::Region& region = volume->getRegion();
	const int32_t lowerX = region.getLowerX();
	const int32_t lowerY = region.getLowerY();
	const int32_t lowerZ = region.getLowerZ();
	const int32_t upperX = region.getUpperX();
	const int32_t upperY = region.getUpperY();
	const int32_t upperZ = region.getUpperZ();
	glm::ivec3 newMins(std::numeric_limits<int>::max());
	glm::ivec3 newMaxs(std::numeric_limits<int>::min());
	voxel::RawVolume::Sampler volumeSampler(volume);
	for (int32_t z = lowerZ; z <= upperZ; ++z) {
		for (int32_t y = lowerY; y <= upperY; ++y) {
			for (int32_t x = lowerX; x <= upperX; ++x) {
				volumeSampler.setPosition(x, y, z);
				const voxel::Voxel& voxel = volumeSampler.getVoxel();
				if (condition(voxel)) {
					continue;
				}
				newMins.x = glm::min(newMins.x, x);
				newMins.y = glm::min(newMins.y, y);
				newMins.z = glm::min(newMins.z, z);

				newMaxs.x = glm::max(newMaxs.x, x);
				newMaxs.y = glm::max(newMaxs.y, y);
				newMaxs.z = glm::max(newMaxs.z, z);
			}
		}
	}
	if (newMaxs.z == std::numeric_limits<int>::min()) {
		return nullptr;
	}
	return cropVolume(volume, newMins, newMaxs, condition);
}

}
