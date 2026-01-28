/**
 * @file
 */

#include "VolumeCropper.h"
#include "core/Algorithm.h"
#include "core/Common.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeMerger.h"

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

	const voxel::Region &region = volume->region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();
	const int yStride = width;
	const int zStride = width * height;
	const voxel::Voxel *data = volume->voxels();
	const size_t lineSize = sizeof(voxel::Voxel) * width;

	int minZ = depth;
	int maxZ = -1;
	int minY = height;
	int maxY = -1;
	int minX = width;
	int maxX = -1;

	// Scan through all Z-Y planes to find bounds
	for (int z = 0; z < depth; ++z) {
		const int zBase = z * zStride;
		for (int y = 0; y < height; ++y) {
			const int baseIndex = zBase + y * yStride;
			const voxel::Voxel *lineStart = &data[baseIndex];

			// Check if this line has any non-air voxels
			const void *found = core::memchr_not(lineStart, 0, lineSize);
			if (found != nullptr) {
				// This line has solid voxels - update Z and Y bounds
				minZ = core_min(minZ, z);
				maxZ = core_max(maxZ, z);
				minY = core_min(minY, y);
				maxY = core_max(maxY, y);

				// Find X bounds within this line
				// Calculate the position of the first non-zero byte
				const uint8_t *lineBytes = (const uint8_t *)lineStart;
				const uint8_t *foundByte = (const uint8_t *)found;
				int firstByteOffset = (int)(foundByte - lineBytes);
				int firstX = firstByteOffset / (int)sizeof(voxel::Voxel);
				minX = core_min(minX, firstX);

				// Find the last non-air voxel in this line by scanning from the end
				for (int x = width - 1; x >= maxX; --x) {
					const voxel::Voxel *voxel = &lineStart[x];
					if (core::memchr_not(voxel, 0, sizeof(voxel::Voxel)) != nullptr) {
						maxX = core_max(maxX, x);
						break;
					}
				}
			}
		}
	}

	if (maxZ < 0) {
		// No solid voxels found
		return nullptr;
	}

	// Convert from local coordinates back to world coordinates
	const glm::ivec3 &lower = region.getLowerCorner();
	glm::ivec3 newMins(lower.x + minX, lower.y + minY, lower.z + minZ);
	glm::ivec3 newMaxs(lower.x + maxX, lower.y + maxY, lower.z + maxZ);

	return cropVolume(volume, newMins, newMaxs);
}
} // namespace voxelutil
