/**
 * @file
 */

#pragma once

#include "voxel/BiomeManager.h"
#include "TreeGenerator.h"
#include "CloudGenerator.h"
#include "core/Trace.h"
#include "noise/SimplexNoise.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/WorldContext.h"
#include "voxel/MaterialColor.h"

namespace voxel {
namespace world {

constexpr int WORLDGEN_TREES = 1 << 0;
constexpr int WORLDGEN_CLOUDS = 1 << 1;

constexpr int WORLDGEN_CLIENT = WORLDGEN_TREES | WORLDGEN_CLOUDS;
constexpr int WORLDGEN_SERVER = WORLDGEN_TREES;

extern int fillVoxels(int x, int z, const WorldContext& worldCtx, Voxel* voxels, BiomeManager& biomManager, long seed, int noiseSeedOffsetX, int noiseSeedOffsetZ);

template<class Volume>
extern void createWorld(const WorldContext& worldCtx, Volume& volume, BiomeManager& biomManager, long seed, int flags, int noiseSeedOffsetX, int noiseSeedOffsetZ) {
	core_trace_scoped(WorldGeneration);
	const Region& region = volume.getRegion();
	// TODO: find a better way to add the current chunk to the seed
	core::Random random(seed + region.getLowerCorner().x * region.getLowerCorner().z);
	Log::debug("Create new chunk at %i:%i:%i", region.getCentreX(), region.getCentreY(), region.getCentreZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int lowerX = region.getLowerX();
	const int lowerZ = region.getLowerZ();
	core_assert(region.getLowerY() >= 0);
	Voxel voxels[MAX_TERRAIN_HEIGHT];

	for (int z = lowerZ; z < lowerZ + depth; ++z) {
		for (int x = lowerX; x < lowerX + width; ++x) {
			const int ni = fillVoxels(x, z, worldCtx, voxels, biomManager, seed, noiseSeedOffsetX, noiseSeedOffsetZ);
			volume.setVoxels(x, z, voxels, ni);
		}
	}
	if ((flags & WORLDGEN_CLOUDS) != 0) {
		core_trace_scoped(Clouds);
		voxel::cloud::CloudContext cloudCtx;
		voxel::cloud::createClouds(volume, biomManager, cloudCtx, random);
	}
	if ((flags & WORLDGEN_TREES) != 0) {
		core_trace_scoped(Trees);
		voxel::tree::createTrees(volume, region, biomManager, random);
	}
}

}
}
