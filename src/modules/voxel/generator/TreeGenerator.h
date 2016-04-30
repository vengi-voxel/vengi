#pragma once

#include "ShapeGenerator.h"

namespace voxel {

class TreeGenerator {
private:
	static int findFloor(const PagedVolume* volume, int x, int y);
public:
	static void createTrees(TerrainContext& ctx, core::Random& random);
	static void addTree(TerrainContext& ctx, const glm::ivec3& pos, TreeType type,
			int trunkHeight, int trunkWidth, int width, int depth, int height, core::Random& random);
};

}
