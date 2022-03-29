/**
 * @file
 */

#include "VolumeResizer.h"
#include "voxelutil/VolumeMerger.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxelutil {

voxel::RawVolume* resize(const voxel::RawVolume* source, const glm::ivec3& size, bool extendMins) {
	voxel::Region region = source->region();
	region.shiftUpperCorner(size);
	if (extendMins) {
		region.shiftLowerCorner(size);
	}
	if (!region.isValid()) {
		return nullptr;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	const voxel::Region& srcRegion = source->region();
	voxel::RawVolumeWrapper wrapper(newVolume);
	voxelutil::mergeVolumes(&wrapper, source, srcRegion, srcRegion);
	return newVolume;
}

}
