#pragma once

#include "ShapeGenerator.h"
#include "voxel/BiomeManager.h"

namespace voxel {

class TreeGenerator {
private:
	static int findFloor(const TerrainContext& ctx, int x, int y);
public:
	static void createTrees(TerrainContext& ctx, const BiomeManager& biomManager, core::Random& random);
	static void addTree(TerrainContext& ctx, const glm::ivec3& pos, TreeType type,
			int trunkHeight, int trunkWidth, int width, int depth, int height, core::Random& random);
};

}
