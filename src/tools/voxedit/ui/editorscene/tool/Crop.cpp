#include "Crop.h"
#include "core/Log.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/VolumeCropper.h"

namespace voxedit {
namespace tool {

voxel::RawVolume* crop(const voxel::RawVolume* source) {
	voxel::RawVolume* newVolume = voxel::cropVolume(source, voxel::createVoxel(voxel::VoxelType::Air));
	if (newVolume == nullptr) {
		Log::info("Failed to crop the model volume");
		return nullptr;
	}
	const glm::ivec3& oldMaxs = source->getRegion().getUpperCorner();
	const glm::ivec3& newMaxs = newVolume->getRegion().getUpperCorner();
	const glm::ivec3 delta = oldMaxs - newMaxs;
	const voxel::Region srcRegion(glm::ivec3(0), delta);
	const voxel::Region& destRegion = newVolume->getRegion();
	voxel::mergeRawVolumes(newVolume, source, destRegion, srcRegion);
	return newVolume;
}

}
}
