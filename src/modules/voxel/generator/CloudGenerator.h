/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"
#include "voxel/BiomeManager.h"

namespace voxel {

class CloudGenerator {
public:
	static glm::ivec2 randomPosWithoutHeight(const Region& region, int border, core::Random& random);
	static void createClouds(TerrainContext& ctx, const BiomeManager& biomManager, core::Random& random);
};

}
