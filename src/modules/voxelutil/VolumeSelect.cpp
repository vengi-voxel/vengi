/**
 * @file
 */

#include "VolumeSelect.h"
#include "core/Common.h"
#include "voxel/RawVolume.h"

namespace voxelutil {

voxel::Region regionForFlag(const voxel::RawVolume &volume, uint8_t flag) {
	const voxel::Region &region = volume.region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();
	const int64_t yStride = width;
	const int64_t zStride = (int64_t)width * height;
	const voxel::Voxel *data = volume.voxels();

	// Flags are at bits 2-3 in the first byte of the 4-byte Voxel struct
	const uint32_t flagsMask32 = (uint32_t)(flag & 0x3) << 2;
	const uint64_t flagsMask64 = ((uint64_t)flagsMask32 << 32) | flagsMask32;

	int minZ = depth;
	int maxZ = -1;
	int minY = height;
	int maxY = -1;
	int minX = width;
	int maxX = -1;

	for (int z = 0; z < depth; ++z) {
		const int64_t zBase = z * zStride;
		for (int y = 0; y < height; ++y) {
			const int64_t baseIndex = zBase + y * yStride;

			// Quick scan: check if this line has any flagged voxels using 64-bit ops
			bool lineHasFlag = false;
			int offset = 0;
			int remaining = width;

			if ((baseIndex & 1) && remaining > 0) {
				const uint32_t *data32 = (const uint32_t *)&data[baseIndex];
				if (*data32 & flagsMask32) {
					lineHasFlag = true;
				}
				offset = 1;
				remaining--;
			}

			if (!lineHasFlag) {
				const int pairs = remaining / 2;
				const uint64_t *data64 = (const uint64_t *)&data[baseIndex + offset];
				for (int i = 0; i < pairs; ++i) {
					if (data64[i] & flagsMask64) {
						lineHasFlag = true;
						break;
					}
				}
			}

			if (!lineHasFlag && (remaining & 1)) {
				const uint32_t *data32 = (const uint32_t *)&data[baseIndex + offset + (remaining / 2) * 2];
				if (*data32 & flagsMask32) {
					lineHasFlag = true;
				}
			}

			if (!lineHasFlag) {
				continue;
			}

			minZ = core_min(minZ, z);
			maxZ = core_max(maxZ, z);
			minY = core_min(minY, y);
			maxY = core_max(maxY, y);

			// Find first flagged voxel in this line
			for (int x = 0; x < minX; ++x) {
				if (data[baseIndex + x].getFlags() & flag) {
					minX = x;
					break;
				}
			}

			// Find last flagged voxel in this line
			for (int x = width - 1; x > maxX; --x) {
				if (data[baseIndex + x].getFlags() & flag) {
					maxX = x;
					break;
				}
			}
		}
	}

	if (maxZ < 0) {
		return voxel::Region::InvalidRegion;
	}

	const glm::ivec3 &lower = region.getLowerCorner();
	return voxel::Region(lower.x + minX, lower.y + minY, lower.z + minZ,
						 lower.x + maxX, lower.y + maxY, lower.z + maxZ);
}

bool lassoContains(const core::DynamicArray<glm::ivec3> &path, int pu, int pv, int uAxis, int vAxis) {
	const int n = (int)path.size();
	bool inside = false;
	for (int i = 0, j = n - 1; i < n; j = i++) {
		const double xi = (double)path[i][uAxis];
		const double yi = (double)path[i][vAxis];
		const double xj = (double)path[j][uAxis];
		const double yj = (double)path[j][vAxis];
		const double px = (double)pu;
		const double py = (double)pv;
		if (((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) {
			inside = !inside;
		}
	}
	return inside;
}

} // namespace voxelutil
