/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"
#include "voxel/BiomeManager.h"
#include "core/Random.h"

namespace voxel {

class GeneratorContext;

class CloudGenerator {
public:
	static glm::ivec2 randomPosWithoutHeight(const Region& region, int border, core::Random& random);
	static void createClouds(GeneratorContext& ctx, const BiomeManager& biomManager, core::Random& random);
};

}
