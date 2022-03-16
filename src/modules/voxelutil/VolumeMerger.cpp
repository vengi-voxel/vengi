/**
 * @file
 */

#include "VolumeMerger.h"
#include <limits>
#include "core/GLM.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"
#include <glm/common.hpp>

namespace voxelutil {

voxel::RawVolume* merge(const core::DynamicArray<const voxel::RawVolume*>& volumes) {
	glm::ivec3 mins((std::numeric_limits<int32_t>::max)() / 2);
	glm::ivec3 maxs((std::numeric_limits<int32_t>::min)() / 2);
	for (const voxel::RawVolume* v : volumes) {
		const voxel::Region& region = v->region();
		mins = (glm::min)(mins, region.getLowerCorner());
		maxs = (glm::max)(maxs, region.getUpperCorner());
	}

	const voxel::Region mergedRegion(mins, maxs);
	Log::debug("Starting to merge volumes into one: %i:%i:%i - %i:%i:%i",
			mergedRegion.getLowerX(), mergedRegion.getLowerY(), mergedRegion.getLowerZ(),
			mergedRegion.getUpperX(), mergedRegion.getUpperY(), mergedRegion.getUpperZ());
	Log::debug("Mins: %i:%i:%i Maxs %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	voxel::RawVolume* merged = new voxel::RawVolume(mergedRegion);
	for (const voxel::RawVolume* v : volumes) {
		const voxel::Region& sr = v->region();
		voxelutil::mergeVolumes(merged, v, sr, sr);
	}
	return merged;
}

voxel::RawVolume* merge(const core::DynamicArray<voxel::RawVolume*>& volumes) {
	core::DynamicArray<const voxel::RawVolume*> v;
	v.reserve(volumes.size());
	for (const voxel::RawVolume *v1 : volumes) {
		v.push_back(v1);
	}
	return merge(v);
}

}
