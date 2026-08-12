/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "core/collection/Array3DView.h"
#include "core/collection/Buffer.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

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
	if (width <= 0 || height <= 0 || depth <= 0) {
		return;
	}
	const int64_t size = (int64_t)width * height * depth;
	const int strideY = width;
	const int strideZ = width * height;
	core::Buffer<bool> visitedData(size);
	core::Array3DView<bool> visited(visitedData.data(), width, height, depth);
	bool *visitedRaw = visitedData.data();

	auto fnWidth = [&volume, &visited, region, depth, height](int start, int end) {
		typename VOLUME::Sampler sampler(&volume);
		sampler.setPosition(region.getLowerX() + start, region.getLowerY(), region.getLowerZ());
		for (int x = start; x < end; ++x) {
			typename VOLUME::Sampler samplerMinY = sampler;
			typename VOLUME::Sampler samplerMaxY = sampler;
			samplerMaxY.movePositiveY(region.getHeightInCells());
			for (int z = 1; z < depth - 1; ++z) {
				samplerMinY.movePositiveZ();
				samplerMaxY.movePositiveZ();
				const voxel::VoxelType m1 = samplerMinY.voxel().getMaterial();
				if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
					visited.set(x, 0, z, true);
				}
				const voxel::VoxelType m2 = samplerMaxY.voxel().getMaterial();
				if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
					visited.set(x, height - 1, z, true);
				}
			}
			typename VOLUME::Sampler samplerMinZ = sampler;
			typename VOLUME::Sampler samplerMaxZ = sampler;
			samplerMaxZ.movePositiveZ(region.getDepthInCells());
			for (int y = 0; y < height; ++y) {
				const voxel::VoxelType m1 = samplerMinZ.voxel().getMaterial();
				if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
					visited.set(x, y, 0, true);
				}
				const voxel::VoxelType m2 = samplerMaxZ.voxel().getMaterial();
				if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
					visited.set(x, y, depth - 1, true);
				}
				samplerMinZ.movePositiveY();
				samplerMaxZ.movePositiveY();
			}
			sampler.movePositiveX();
		}
	};
	app::for_parallel(0, width, fnWidth);

	auto fnHeight = [&volume, &visited, width, depth, region](int start, int end) {
		for (int y = start; y < end; ++y) {
			typename VOLUME::Sampler samplerMinX(&volume);
			typename VOLUME::Sampler samplerMaxX(&volume);
			samplerMinX.setPosition(region.getLowerX(), region.getLowerY() + y, region.getLowerZ() + 1);
			samplerMaxX.setPosition(region.getLowerX() + width - 1, region.getLowerY() + y, region.getLowerZ() + 1);
			for (int z = 1; z < depth - 1; ++z) {
				const voxel::VoxelType m1 = samplerMinX.voxel().getMaterial();
				if (voxel::isAir(m1) || voxel::isTransparent(m1)) {
					visited.set(0, y, z, true);
				}
				const voxel::VoxelType m2 = samplerMaxX.voxel().getMaterial();
				if (voxel::isAir(m2) || voxel::isTransparent(m2)) {
					visited.set(width - 1, y, z, true);
				}
				samplerMinX.movePositiveZ();
				samplerMaxX.movePositiveZ();
			}
		}
	};
	if (height > 2) {
		app::for_parallel(1, height - 1, fnHeight);
	}

	/* Seed flood-fill from outside air on the border only (O(surface), not O(volume)).
	 * Reserve for the worst case: exterior flood can touch nearly every air voxel. */
	core::Buffer<int> positions;
	positions.reserve((size_t)size);
	auto pushIfVisited = [visitedRaw, strideY, strideZ, &positions](int x, int y, int z) {
		const int idx = x + y * strideY + z * strideZ;
		if (visitedRaw[idx]) {
			positions.push_back(idx);
		}
	};
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			pushIfVisited(x, y, 0);
			if (depth > 1) {
				pushIfVisited(x, y, depth - 1);
			}
		}
	}
	for (int z = 1; z < depth - 1; ++z) {
		for (int x = 0; x < width; ++x) {
			pushIfVisited(x, 0, z);
			if (height > 1) {
				pushIfVisited(x, height - 1, z);
			}
		}
	}
	for (int z = 1; z < depth - 1; ++z) {
		for (int y = 1; y < height - 1; ++y) {
			pushIfVisited(0, y, z);
			if (width > 1) {
				pushIfVisited(width - 1, y, z);
			}
		}
	}

	{
		typename VOLUME::Sampler sampler(&volume);
		sampler.setPosition(region.getLowerCorner());
		for (int z = 0; z < depth; ++z) {
			typename VOLUME::Sampler sampler2 = sampler;
			for (int y = 0; y < height; ++y) {
				typename VOLUME::Sampler sampler3 = sampler2;
				for (int x = 0; x < width; ++x) {
					if (voxel::isAir(sampler3.voxel().getMaterial())) {
						sampler3.movePositiveX();
						continue;
					}
					visitedRaw[x + y * strideY + z * strideZ] = true;
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	}

	while (!positions.empty()) {
		const int i = positions.back();
		positions.pop();
		const int x = i % width;
		const int plane = i % strideZ; /* x + y * width within z-slice */

		if (i >= strideZ && !visitedRaw[i - strideZ]) {
			visitedRaw[i - strideZ] = true;
			positions.push_back(i - strideZ);
		}
		if (plane >= strideY && !visitedRaw[i - strideY]) {
			visitedRaw[i - strideY] = true;
			positions.push_back(i - strideY);
		}
		if (x > 0 && !visitedRaw[i - 1]) {
			visitedRaw[i - 1] = true;
			positions.push_back(i - 1);
		}
		if ((int64_t)i + strideZ < size && !visitedRaw[i + strideZ]) {
			visitedRaw[i + strideZ] = true;
			positions.push_back(i + strideZ);
		}
		if (plane + strideY < strideZ && !visitedRaw[i + strideY]) {
			visitedRaw[i + strideY] = true;
			positions.push_back(i + strideY);
		}
		if (x < width - 1 && !visitedRaw[i + 1]) {
			visitedRaw[i + 1] = true;
			positions.push_back(i + 1);
		}
	}

	{
		const bool *dataFinal = visitedRaw;
		typename VOLUME::Sampler sampler(&volume);
		sampler.setPosition(region.getLowerX(), region.getLowerY(), region.getLowerZ());
		for (int z = 0; z < depth; ++z) {
			typename VOLUME::Sampler sampler2 = sampler;
			for (int y = 0; y < height; ++y) {
				typename VOLUME::Sampler sampler3 = sampler2;
				for (int x = 0; x < width; ++x) {
					if (!*dataFinal++) {
						sampler3.setVoxel(voxel);
					}
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	}
}

} // namespace voxelutil
