/**
 * @file
 */

#include "CloudGenerator.h"
#include "voxel/WorldContext.h"

namespace voxel {

glm::ivec2 CloudGenerator::randomPosWithoutHeight(const Region& region, int border, core::Random& random) {
	const int w = region.getWidthInVoxels();
	const int d = region.getDepthInVoxels();
	core_assert(border < w);
	core_assert(border < d);
	const int x = random.random(border, w - border);
	const int z = random.random(border, d - border);
	return glm::ivec2(region.getLowerX() + x, region.getLowerZ() + z);
}

void CloudGenerator::createClouds(TerrainContext& ctx, const BiomeManager& biomManager, core::Random& random) {
	const int amount = 4;
	static constexpr Voxel voxel = createVoxel(Cloud);
	for (int i = 0; i < amount; ++i) {
		const int height = 10;
		const glm::ivec2& pos = randomPosWithoutHeight(ctx.region, 20, random);
		glm::ivec3 chunkCloudCenterPos(pos.x, ctx.region.getUpperY() - height, pos.y);
		if (!biomManager.hasClouds(chunkCloudCenterPos)) {
			continue;
		}
		ShapeGenerator::createEllipse(ctx, chunkCloudCenterPos, 10, height, 10, voxel);
		chunkCloudCenterPos.x -= 5;
		chunkCloudCenterPos.y -= 5 + i;
		ShapeGenerator::createEllipse(ctx, chunkCloudCenterPos, 20, height, 20, voxel);
	}
}

}
