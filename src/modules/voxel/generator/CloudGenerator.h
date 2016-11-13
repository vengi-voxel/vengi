/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"
#include "voxel/BiomeManager.h"
#include "core/Random.h"

namespace voxel {

class PagedVolumeWrapper;

class CloudGenerator {
public:
	static glm::ivec2 randomPosWithoutHeight(const Region& region, int border, core::Random& random);
	static void createClouds(PagedVolumeWrapper& ctx, const BiomeManager& biomManager, core::Random& random);
};

}
