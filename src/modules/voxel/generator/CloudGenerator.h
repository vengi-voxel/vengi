#pragma once

#include "ShapeGenerator.h"
#include "voxel/BiomManager.h"

namespace voxel {

class CloudGenerator {
public:
	static glm::ivec2 randomPosWithoutHeight(const Region& region, int border, core::Random& random);
	static void createClouds(TerrainContext& ctx, const BiomManager& biomManager, core::Random& random);
};

}
