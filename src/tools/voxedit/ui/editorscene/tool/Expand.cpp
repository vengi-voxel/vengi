#include "Expand.h"
#include "voxel/polyvox/VolumeMerger.h"

namespace voxedit {
namespace tool {

voxel::RawVolume* expand(const voxel::RawVolume* source, int size) {
	voxel::Region region = source->getRegion();
	region.shiftUpperCorner(size, size, size);
	if (!region.isValid()) {
		return nullptr;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	const voxel::Region& destRegion = source->getRegion();
	const voxel::Region& srcRegion = source->getRegion();
	voxel::mergeRawVolumes(newVolume, source, destRegion, srcRegion);
	return newVolume;
}

}
}
