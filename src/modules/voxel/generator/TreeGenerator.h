#pragma once

#include "ShapeGenerator.h"
#include "core/Random.h"

namespace voxel {

class TreeGenerator {
private:
	static int findChunkFloor(int chunkSize, PagedVolume::Chunk* chunk, int x, int y);
public:
	static void createTrees(TerrainContext& ctx, core::Random& random);
	static void addTree(TerrainContext& ctx, const glm::ivec3& pos, TreeType type,
			int trunkHeight, int trunkWidth, int width, int depth, int height, core::Random& random);
};

}
