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
};

template<class Volume, class BiomeManager>
bool createClouds(Volume& volume, const voxel::Region& region, const BiomeManager& biomManager, const CloudContext& ctx) {
	core::Random random(region.getCentreX() + region.getCentreY() + region.getCentreZ());
	const Voxel& voxel = createRandomColorVoxel(VoxelType::Cloud, random);

	std::vector<glm::vec2> positions;
	const int bounds = std::max({ctx.width1, ctx.width2, ctx.depth1, ctx.depth2});
	biomManager.getCloudPositions(region, positions, random, bounds);

	for (const glm::vec2& position : positions) {
		glm::ivec3 pos(position.x, region.getUpperY() - ctx.height, position.y);
		shape::createEllipse(volume, pos, ctx.width1, ctx.height, ctx.depth1, voxel);
		pos.x += ctx.deltaX;
		pos.y += ctx.deltaY;
		shape::createEllipse(volume, pos, ctx.width2, ctx.height, ctx.depth2, voxel);
	}
	return !positions.empty();
}

}
}
