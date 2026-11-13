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

// Skip zero-filled air quickly, then check material so air metadata is ignored.
static CORE_FORCE_INLINE bool scanSolidRow(const voxel::Voxel *data, int count, int x, int &minX, int &maxX) {
	const void *found = core::memchr_not(data, 0, (size_t)count * sizeof(voxel::Voxel));
	if (found == nullptr) {
		return false;
	}
	int first = (int)(((const uint8_t *)found - (const uint8_t *)data) / sizeof(voxel::Voxel));
	while (first < count && voxel::isAir(data[first].getMaterial())) {
		++first;
	}
	if (first == count) {
		return false;
	}
	minX = core_min(minX, x + first);
	for (int last = count - 1; x + last > maxX; --last) {
		if (!voxel::isAir(data[last].getMaterial())) {
			maxX = x + last;
			break;
		}
	}
	return true;
}

static bool computeChunkedSolidBounds(const voxel::RawVolume *volume, glm::ivec3 &mins, glm::ivec3 &maxs) {
	const voxel::Region &region = volume->region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();
	int minZ = depth, maxZ = -1;
	int minY = height, maxY = -1;
	int minX = width, maxX = -1;
	const auto includeRows = [&](int y, int z, int rows, int slices) {
		minY = core_min(minY, y);
		maxY = core_max(maxY, y + rows - 1);
		minZ = core_min(minZ, z);
		maxZ = core_max(maxZ, z + slices - 1);
	};
	core::IProgress *progress = core::currentProgressPtr();
	for (int z = 0; z < depth;) {
		int slices = depth - z;
		for (int y = 0; y < height;) {
			int rows = height - y;
			for (int x = 0; x < width;) {
				const glm::ivec3 pos = region.getLowerCorner() + glm::ivec3(x, y, z);
				glm::ivec3 lower, upper;
				int64_t yStride, zStride;
				core::AtomicPtr<voxel::Voxel> *entry;
				const voxel::Voxel *background;
				const voxel::Voxel *data = volume->sampleData(pos, lower, upper, yStride, zStride, entry, background);
				const int count = upper.x - pos.x + 1;
				rows = upper.y - pos.y + 1;
				slices = upper.z - pos.z + 1;
				const auto coveredXY = [&]() {
					return minX <= x && maxX >= x + count - 1 && minY <= y && maxY >= y + rows - 1;
				};
				// An enclosed block cannot expand the bounds, regardless of its contents.
				if (coveredXY() && minZ <= z && maxZ >= z + slices - 1) {
					x += count;
					continue;
				}
				if (data != nullptr) {
					for (int dz = 0; dz < slices; ++dz) {
						if (coveredXY() && minZ <= z + dz && maxZ >= z + dz) {
							continue;
						}
						for (int dy = 0; dy < rows; ++dy) {
							if (scanSolidRow(data + dz * zStride + dy * yStride, count, x, minX, maxX)) {
								includeRows(y + dy, z + dz, 1, 1);
								if (coveredXY()) {
									break;
								}
							}
						}
						if (coveredXY() && minZ <= z && maxZ >= z + slices - 1) {
							break;
						}
					}
				} else if (!voxel::isAir(background->getMaterial())) {
					minX = core_min(minX, x);
					maxX = core_max(maxX, x + count - 1);
					includeRows(y, z, rows, slices);
				}
				x += count;
			}
			y += rows;
		}
		z += slices;
		if (progress != nullptr) {
			progress->setProgress((float)z / (float)depth);
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

[[nodiscard]] bool computeSolidBounds(const voxel::RawVolume *volume, glm::ivec3 &mins, glm::ivec3 &maxs) {
	if (volume == nullptr || !volume->valid()) {
		return false;
	}
	core_trace_scoped(ComputeSolidBounds);
	const voxel::Voxel *data = volume->linearVoxels();
	if (data == nullptr) {
		return computeChunkedSolidBounds(volume, mins, maxs);
	}

	const voxel::Region &region = volume->region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();
	const int64_t yStride = width;
	const int64_t zStride = (int64_t)width * height;
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
				int firstX = (int)(((const uint8_t *)found - (const uint8_t *)lineStart) / sizeof(voxel::Voxel));
				while (firstX < width && voxel::isAir(lineStart[firstX].getMaterial())) {
					++firstX;
				}
				if (firstX == width) {
					continue;
				}
				minZ = core_min(minZ, z);
				maxZ = core_max(maxZ, z);
				minY = core_min(minY, y);
				maxY = core_max(maxY, y);

				minX = core_min(minX, firstX);

				for (int x = width - 1; x > maxX; --x) {
					const voxel::Voxel *candidate = &lineStart[x];
					if (!voxel::isAir(candidate->getMaterial())) {
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
