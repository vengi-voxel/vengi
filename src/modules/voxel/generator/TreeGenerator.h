/**
 * @file
 */

#pragma once

#include "voxel/BiomeManager.h"
#include "core/Random.h"
#include "voxel/WorldContext.h"

namespace voxel {

class GeneratorContext;

namespace tree {

extern void createTrees(GeneratorContext& ctx, const BiomeManager& biomManager, core::Random& random);
extern void addTree(GeneratorContext& ctx, const glm::ivec3& pos, TreeType type,
			int trunkHeight, int trunkWidth, int width, int depth, int height, core::Random& random);

}
}
