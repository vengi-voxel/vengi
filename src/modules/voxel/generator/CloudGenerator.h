/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"
#include "voxel/BiomeManager.h"
#include "core/Random.h"

namespace voxel {
namespace cloud {

struct CloudContext {
	int amount = 4;
	int height = 10;
	int width1 = 10;
	int width2 = 20;
	int depth1 = 10;
	int depth2 = 20;
	int deltaX = -5;
	int deltaY = -5;
	int regionBorder = 20;
	bool randomPos = true;
	glm::ivec3 pos;
};

static inline glm::ivec2 randomPosWithoutHeight(const Region& region, int border, core::Random& random) {
	const int w = region.getWidthInVoxels();
	const int d = region.getDepthInVoxels();
	core_assert(border < w);
	core_assert(border < d);
	const int x = random.random(border, w - border);
	const int z = random.random(border, d - border);
	return glm::ivec2(region.getLowerX() + x, region.getLowerZ() + z);
}

template<class Volume, class BiomeManager>
void createClouds(Volume& volume, const BiomeManager& biomManager, const CloudContext& ctx, core::Random& random) {
	const Voxel& voxel = createRandomColorVoxel(VoxelType::Cloud, random);
	const voxel::Region& region = volume.getRegion();
	glm::ivec3 chunkCloudCenterPos = ctx.pos;
	for (int i = 0; i < ctx.amount; ++i) {
		if (ctx.randomPos) {
			const glm::ivec2& pos = randomPosWithoutHeight(region, ctx.regionBorder, random);
			chunkCloudCenterPos = glm::ivec3(pos.x, region.getUpperY() - ctx.height, pos.y);
		}
		if (!biomManager.hasClouds(chunkCloudCenterPos)) {
			continue;
		}
		shape::createEllipse(volume, chunkCloudCenterPos, ctx.width1, ctx.height, ctx.depth1, voxel);
		chunkCloudCenterPos.x += ctx.deltaX;
		chunkCloudCenterPos.y += ctx.deltaY + i;
		shape::createEllipse(volume, chunkCloudCenterPos, ctx.width2, ctx.height, ctx.depth2, voxel);
		if (!ctx.randomPos) {
			chunkCloudCenterPos.x += ctx.deltaX;
			chunkCloudCenterPos.y += ctx.deltaY;
		}
	}
}

}
}
