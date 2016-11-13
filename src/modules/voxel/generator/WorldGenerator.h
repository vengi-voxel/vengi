/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"
#include "voxel/BiomeManager.h"

namespace voxel {

class PagedVolumeWrapper;
struct WorldContext;

constexpr int WORLDGEN_TREES = 1 << 0;
constexpr int WORLDGEN_CLOUDS = 1 << 1;

constexpr int WORLDGEN_CLIENT = WORLDGEN_TREES | WORLDGEN_CLOUDS;
constexpr int WORLDGEN_SERVER = WORLDGEN_TREES;

class WorldGenerator {
public:
	static void createWorld(WorldContext& worldCtx, PagedVolumeWrapper& ctx, BiomeManager& biomManager, long seed, int flags, int noiseSeedOffsetX, int noiseSeedOffsetZ);
};

}
