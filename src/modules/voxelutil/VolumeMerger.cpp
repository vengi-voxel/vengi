/**
 * @file
 */

#include "VolumeMerger.h"
#include <limits>
#include "core/GLM.h"
#include "core/Log.h"
#include <glm/common.hpp>

namespace voxel {

RawVolume* merge(const std::vector<const RawVolume*>& volumes) {
	glm::ivec3 mins((std::numeric_limits<int32_t>::max)());
	glm::ivec3 maxs((std::numeric_limits<int32_t>::min)());
	for (const voxel::RawVolume* v : volumes) {
		const voxel::Region& region = v->region();
		mins = (glm::min)(mins, region.getLowerCorner());
		maxs = (glm::max)(maxs, region.getUpperCorner());
	}

	const voxel::Region mergedRegion(glm::ivec3(0), maxs - mins);
	Log::debug("Starting to merge volumes into one: %i:%i:%i - %i:%i:%i",
			mergedRegion.getLowerX(), mergedRegion.getLowerY(), mergedRegion.getLowerZ(),
			mergedRegion.getUpperX(), mergedRegion.getUpperY(), mergedRegion.getUpperZ());
	Log::debug("Mins: %i:%i:%i Maxs %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	voxel::RawVolume* merged = new voxel::RawVolume(mergedRegion);
	for (const voxel::RawVolume* v : volumes) {
		const voxel::Region& sr = v->region();
		const glm::ivec3& destMins = sr.getLowerCorner() - mins;
		const voxel::Region dr(destMins, destMins + sr.getDimensionsInCells());
		Log::debug("Merge %i:%i:%i - %i:%i:%i into %i:%i:%i - %i:%i:%i",
				sr.getLowerX(), sr.getLowerY(), sr.getLowerZ(),
				sr.getUpperX(), sr.getUpperY(), sr.getUpperZ(),
				dr.getLowerX(), dr.getLowerY(), dr.getLowerZ(),
				dr.getUpperX(), dr.getUpperY(), dr.getUpperZ());
		voxel::mergeVolumes(merged, v, dr, sr);
	}
	return merged;
}

}
