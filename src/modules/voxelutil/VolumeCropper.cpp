/**
 * @file
 */

#include "VolumeCropper.h"
#include "core/Common.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

[[nodiscard]] voxel::RawVolume *cropVolume(const voxel::RawVolume *volume, const glm::ivec3 &mins,
										   const glm::ivec3 &maxs) {
	core_trace_scoped(CropVolume);
	const voxel::Region newRegion(mins, maxs);
	if (!newRegion.isValid()) {
		return nullptr;
	}
	if (newRegion == volume->region()) {
		return nullptr;
	}
	voxel::RawVolume *newVolume = new voxel::RawVolume(newRegion);
	voxelutil::mergeVolumes(newVolume, volume, newRegion, voxel::Region(mins, maxs));
	return newVolume;
}

[[nodiscard]] voxel::RawVolume *cropVolume(const voxel::RawVolume *volume) {
	if (volume == nullptr) {
		return nullptr;
	}
	core_trace_scoped(CropVolume);
	glm::ivec3 newMins((std::numeric_limits<int>::max)() / 2);
	glm::ivec3 newMaxs((std::numeric_limits<int>::min)() / 2);
	auto visitor = [&newMins, &newMaxs](int x, int y, int z, const voxel::Voxel &) {
		newMins.x = core_min(newMins.x, x);
		newMins.y = core_min(newMins.y, y);
		newMins.z = core_min(newMins.z, z);

		newMaxs.x = core_max(newMaxs.x, x);
		newMaxs.y = core_max(newMaxs.y, y);
		newMaxs.z = core_max(newMaxs.z, z);
	};
	// TODO: PERF: this algorithm can be optimized by using core::memchr_not
	if (visitVolume(*volume, visitor, VisitSolid()) == 0) {
		return nullptr;
	}
	return cropVolume(volume, newMins, newMaxs);
}
} // namespace voxelutil
