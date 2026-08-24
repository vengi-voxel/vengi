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
	const int64_t yStride = width;
	const int64_t zStride = (int64_t)width * height;
	const voxel::Voxel *data = volume->voxels();
	const size_t lineSize = sizeof(voxel::Voxel) * width;

	int minZ = depth;
	int maxZ = -1;
	int minY = height;
	int maxY = -1;
	int minX = width;
	int maxX = -1;

	core::IProgress *progress = core::currentProgressPtr();
	for (int z = 0; z < depth; ++z) {
		const int64_t zBase = z * zStride;
		for (int y = 0; y < height; ++y) {
			const int64_t baseIndex = zBase + y * yStride;
			const voxel::Voxel *lineStart = &data[baseIndex];

			const void *found = core::memchr_not(lineStart, 0, lineSize);
			if (found != nullptr) {
				minZ = core_min(minZ, z);
				maxZ = core_max(maxZ, z);
				minY = core_min(minY, y);
				maxY = core_max(maxY, y);

				const uint8_t *lineBytes = (const uint8_t *)lineStart;
				const uint8_t *foundByte = (const uint8_t *)found;
				const int firstByteOffset = (int)(foundByte - lineBytes);
				const int firstX = firstByteOffset / (int)sizeof(voxel::Voxel);
				minX = core_min(minX, firstX);

				for (int x = width - 1; x >= maxX; --x) {
					const voxel::Voxel *voxel = &lineStart[x];
					if (core::memchr_not(voxel, 0, sizeof(voxel::Voxel)) != nullptr) {
						maxX = core_max(maxX, x);
						break;
					}
				}
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
