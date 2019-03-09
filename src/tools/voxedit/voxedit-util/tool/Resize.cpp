/**
 * @file
 */

#include "Resize.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/RawVolumeWrapper.h"

namespace voxedit {
namespace tool {

voxel::RawVolume* resize(const voxel::RawVolume* source, const glm::ivec3& size) {
	voxel::Region region = source->region();
	region.shiftUpperCorner(size);
	if (!region.isValid()) {
		return nullptr;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	const voxel::Region& destRegion = source->region();
	const voxel::Region& srcRegion = source->region();
	voxel::RawVolumeWrapper wrapper(newVolume);
	voxel::mergeVolumes(&wrapper, source, destRegion, srcRegion);
	return newVolume;
}

}
}
