/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"
#include "voxel/BiomeManager.h"
#include "math/Random.h"
#include "SpaceColonization.h"

namespace voxel {
namespace cloud {

struct CloudContext {
	int height = 26;
	int width = 80;
	int depth = 80;
};

template<class Volume, class BiomeManager>
bool createClouds(Volume& volume, const voxel::Region& region, const BiomeManager& biomManager, const CloudContext& ctx) {
	math::Random random(region.getCentreX() + region.getCentreY() + region.getCentreZ());

	std::vector<glm::vec2> positions;
	biomManager.getCloudPositions(region, positions, random, std::max(ctx.width, ctx.depth));

	const Voxel& voxel = createRandomColorVoxel(VoxelType::Cloud, random);
	SpaceColonization::RandomSize rndSize(random);

	for (const glm::vec2& position : positions) {
		const glm::ivec3 pos(position.x, region.getUpperY() - ctx.height, position.y);
		SpaceColonization sc(pos, 6, ctx.width, ctx.height, ctx.depth, 1.0f, random.seed(), 2, 8, 30);
		sc.grow();
		sc.generateLeaves(volume, voxel, rndSize);
	}
	return !positions.empty();
}

}
}
