/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"
#include "voxel/BiomeManager.h"
#include "core/Random.h"

namespace voxel {
namespace cloud {

static glm::ivec2 randomPosWithoutHeight(const Region& region, int border, core::Random& random) {
	const int w = region.getWidthInVoxels();
	const int d = region.getDepthInVoxels();
	core_assert(border < w);
	core_assert(border < d);
	const int x = random.random(border, w - border);
	const int z = random.random(border, d - border);
	return glm::ivec2(region.getLowerX() + x, region.getLowerZ() + z);
}

template<class Volume>
void createClouds(Volume& ctx, const BiomeManager& biomManager, core::Random& random) {
	const int amount = 4;
	static constexpr Voxel voxel = createVoxel(VoxelType::Cloud);
	const voxel::Region& region = ctx.getRegion();
	for (int i = 0; i < amount; ++i) {
		const int height = 10;
		const glm::ivec2& pos = randomPosWithoutHeight(region, 20, random);
		glm::ivec3 chunkCloudCenterPos(pos.x, region.getUpperY() - height, pos.y);
		if (!biomManager.hasClouds(chunkCloudCenterPos)) {
			continue;
		}
		shape::createEllipse(ctx, chunkCloudCenterPos, 10, height, 10, voxel);
		chunkCloudCenterPos.x -= 5;
		chunkCloudCenterPos.y -= 5 + i;
		shape::createEllipse(ctx, chunkCloudCenterPos, 20, height, 20, voxel);
	}
}

}
}
