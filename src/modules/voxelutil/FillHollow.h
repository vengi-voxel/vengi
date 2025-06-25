/**
 * @file
 */

#pragma once

#include "core/collection/Array3DView.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

/**
 * @brief Fills the hollow spaces in a voxel volume.
 *
 * This function iterates over the voxel volume and identifies hollows that are totally enclosed by existing voxels.
 * It then fills these hollow spaces with a specified voxel.
 *
 * @param[in,out] volume The voxel volume to fill.
 * @param[in] voxel The voxel to fill the hollow spaces with.
 */
template<class VOLUME>
void fillHollow(VOLUME &volume, const voxel::Voxel &voxel) {
	const voxel::Region &region = volume.region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();
	const int size = width * height * depth;
	const glm::ivec3 mins = volume.region().getLowerCorner();
	core::Buffer<glm::ivec3> positions;
	positions.reserve(size);
	core::Buffer<bool> visitedData(size);
	core::Array3DView<bool> visited(visitedData.data(), width, height, depth);

	visitVolume(
		volume, region, 1, 1, 1,
		[&](int x, int y, int z, const voxel::Voxel &) { visited.set(x - mins.x, y - mins.y, z - mins.z, true); },
		SkipEmpty());

	// TODO: PERF: use volume samplers
	for (int x = 0; x < width; ++x) {
		for (int z = 1; z < depth - 1; ++z) {
			const glm::ivec3 v1(x, 0, z);
			const voxel::VoxelType m1 = volume.voxel(v1 + mins).getMaterial();
			if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(x, height - 1, z);
			const voxel::VoxelType m2 = volume.voxel(v2 + mins).getMaterial();
			if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
		for (int y = 0; y < height; ++y) {
			const glm::ivec3 v1(x, y, 0);
			const voxel::VoxelType m1 = volume.voxel(v1 + mins).getMaterial();
			if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(x, y, depth - 1);
			const voxel::VoxelType m2 = volume.voxel(v2 + mins).getMaterial();
			if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
	}
	for (int y = 1; y < height - 1; ++y) {
		for (int z = 1; z < depth - 1; ++z) {
			const glm::ivec3 v1(0, y, z);
			const voxel::VoxelType m1 = volume.voxel(v1 + mins).getMaterial();
			if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
				positions.push_back(v1);
				visited.set(v1, true);
			}
			const glm::ivec3 v2(width - 1, y, z);
			const voxel::VoxelType m2 = volume.voxel(v2 + mins).getMaterial();
			if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
				positions.push_back(v2);
				visited.set(v2, true);
			}
		}
	}

	while (!positions.empty()) {
		const glm::ivec3 v = positions.back();
		positions.erase(positions.size() - 1);

		if (v.z > 0 && !visited.get(v.x, v.y, v.z - 1)) {
			const glm::ivec3 v1(v.x, v.y, v.z - 1);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.y > 0 && !visited.get(v.x, v.y - 1, v.z)) {
			const glm::ivec3 v1(v.x, v.y - 1, v.z);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.x > 0 && !visited.get(v.x - 1, v.y, v.z)) {
			const glm::ivec3 v1(v.x - 1, v.y, v.z);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.z < depth - 1 && !visited.get(v.x, v.y, v.z + 1)) {
			const glm::ivec3 v1(v.x, v.y, v.z + 1);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.y < height - 1 && !visited.get(v.x, v.y + 1, v.z)) {
			const glm::ivec3 v1(v.x, v.y + 1, v.z);
			visited.set(v1, true);
			positions.push_back(v1);
		}
		if (v.x < width - 1 && !visited.get(v.x + 1, v.y, v.z)) {
			const glm::ivec3 v1(v.x + 1, v.y, v.z);
			visited.set(v1, true);
			positions.push_back(v1);
		}
	}

	auto visitor = [&](int x, int y, int z, const voxel::Voxel &) {
		if (!visited.get(x - mins.x, y - mins.y, z - mins.z)) {
			volume.setVoxel(x, y, z, voxel);
		}
	};
	visitVolume(volume, region, 1, 1, 1, visitor, VisitAll());
}

} // namespace voxelutil
