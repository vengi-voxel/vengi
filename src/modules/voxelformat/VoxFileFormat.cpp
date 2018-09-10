/**
 * @file
 */

#include "VoxFileFormat.h"
#include "voxel/MaterialColor.h"
#include "voxel/polyvox/VolumeMerger.h"
	#include "core/Common.h"
#include "core/Log.h"
#include "core/Color.h"
#include <limits>

namespace voxel {

const glm::vec4& VoxFileFormat::getColor(const Voxel& voxel) const {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	return materialColors[voxel.getColor()];
}

uint8_t VoxFileFormat::convertPaletteIndex(uint32_t paletteIndex) const {
	if (paletteIndex >= _paletteSize) {
		return 1;
	}
	return _palette[paletteIndex];
}

glm::vec4 VoxFileFormat::findClosestMatch(const glm::vec4& color) const {
	const int index = findClosestIndex(color);
	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	return materialColors[index];
}

uint8_t VoxFileFormat::findClosestIndex(const glm::vec4& color) const {
	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	materialColors.erase(materialColors.begin());
	return core::Color::getClosestMatch(color, materialColors);
}

RawVolume* VoxFileFormat::load(const io::FilePtr& file) {
	glm::ivec3 mins(std::numeric_limits<int32_t>::max());
	glm::ivec3 maxs(std::numeric_limits<int32_t>::min());
	std::vector<voxel::RawVolume*> volumes = loadGroups(file);
	for (const voxel::RawVolume* v : volumes) {
		const voxel::Region& region = v->region();
		mins = glm::min(mins, region.getLowerCorner());
		maxs = glm::max(maxs, region.getUpperCorner());
	}

	const voxel::Region mergedRegion(glm::ivec3(0), maxs - mins);
	Log::debug("Starting to merge volumes into one: %i:%i:%i - %i:%i:%i",
			mergedRegion.getLowerX(), mergedRegion.getLowerY(), mergedRegion.getLowerZ(),
			mergedRegion.getUpperX(), mergedRegion.getUpperY(), mergedRegion.getUpperZ());
	Log::debug("Mins: %i:%i:%i Maxs %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	voxel::RawVolume* merged = new voxel::RawVolume(mergedRegion);
	for (voxel::RawVolume* v : volumes) {
		const voxel::Region& sr = v->region();
		const glm::ivec3& destMins = sr.getLowerCorner() - mins;
		const voxel::Region dr(destMins, destMins + sr.getDimensionsInCells());
		Log::debug("Merge %i:%i:%i - %i:%i:%i into %i:%i:%i - %i:%i:%i",
				sr.getLowerX(), sr.getLowerY(), sr.getLowerZ(),
				sr.getUpperX(), sr.getUpperY(), sr.getUpperZ(),
				dr.getLowerX(), dr.getLowerY(), dr.getLowerZ(),
				dr.getUpperX(), dr.getUpperY(), dr.getUpperZ());
		voxel::mergeVolumes(merged, v, dr, sr);
		delete v;
	}
	return merged;
}


}
