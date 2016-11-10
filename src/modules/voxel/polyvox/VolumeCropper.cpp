#include "VolumeCropper.h"
#include "VolumeMerger.h"
#include "core/Log.h"
#include "core/Trace.h"
#include <limits>

namespace voxel {

RawVolume* cropVolume(const RawVolume* volume, const Voxel& emptyVoxel) {
	core_trace_scoped(CropRawVolume);
	const voxel::Region& region = volume->getEnclosingRegion();
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
				const voxel::Voxel& material = volumeSampler.getVoxel();
				if (material == emptyVoxel) {
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
	const voxel::Region newRegion(glm::ivec3(), newMaxs - newMins);
	if (!newRegion.isValid()) {
		return nullptr;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(newRegion);
	voxel::mergeRawVolumes(newVolume, volume, newRegion, voxel::Region(newMins, newMaxs));
	return newVolume;
}

}
