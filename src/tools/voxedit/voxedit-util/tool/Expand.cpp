#include "Expand.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/RawVolumeWrapper.h"

namespace voxedit {
namespace tool {

voxel::RawVolume* expand(const voxel::RawVolume* source, const glm::ivec3& size) {
	voxel::Region region = source->getRegion();
	region.shiftUpperCorner(size);
	if (!region.isValid()) {
		return nullptr;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	const voxel::Region& destRegion = source->getRegion();
	const voxel::Region& srcRegion = source->getRegion();
	voxel::RawVolumeWrapper wrapper(newVolume);
	voxel::mergeVolumes(&wrapper, source, destRegion, srcRegion);
	return newVolume;
}

}
}
