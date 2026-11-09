/**
 * @file
 */

#include "VolumeCropper.h"
#include "core/Algorithm.h"
#include "core/Common.h"
#include "core/ProgressScope.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeMerger.h"

namespace voxelutil {

[[nodiscard]] bool computeSolidBounds(const voxel::RawVolume *volume, glm::ivec3 &mins, glm::ivec3 &maxs) {
	if (volume == nullptr) {
		return false;
	}
	core_trace_scoped(ComputeSolidBounds);

	const voxel::Region &region = volume->region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();

	int minZ = depth;
	int maxZ = -1;
	int minY = height;
	int maxY = -1;
	int minX = width;
	int maxX = -1;

	core::IProgress *progress = core::currentProgressPtr();
	voxel::RawVolume::Sampler sampler(volume);
	for (int z = 0; z < depth; ++z) {
		for (int y = 0; y < height; ++y) {
			sampler.setPosition(region.getLowerX(), region.getLowerY() + y, region.getLowerZ() + z);
			for (int x = 0; x < width; ++x) {
				if (!voxel::isAir(sampler.voxel().getMaterial())) {
					minX = core_min(minX, x);
					maxX = core_max(maxX, x);
					minY = core_min(minY, y);
					maxY = core_max(maxY, y);
					minZ = core_min(minZ, z);
					maxZ = core_max(maxZ, z);
				}
				sampler.movePositiveX();
			}
		}
		if (progress != nullptr && depth > 0) {
			progress->setProgress(((float)(z + 1)) / (float)depth);
		}
	}

	if (maxZ < 0) {
		return false;
	}

	const glm::ivec3 &lower = region.getLowerCorner();
	mins = glm::ivec3(lower.x + minX, lower.y + minY, lower.z + minZ);
	maxs = glm::ivec3(lower.x + maxX, lower.y + maxY, lower.z + maxZ);
	return true;
}

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
	glm::ivec3 mins;
	glm::ivec3 maxs;
	if (!computeSolidBounds(volume, mins, maxs)) {
		return nullptr;
	}
	return cropVolume(volume, mins, maxs);
}
} // namespace voxelutil
