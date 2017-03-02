/**
 * @file
 */

#pragma once

#include "voxel/BiomeManager.h"
#include "TreeGenerator.h"
#include "CloudGenerator.h"
#include "BuildingGenerator.h"
#include "core/Trace.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/WorldContext.h"
#include "voxel/MaterialColor.h"

namespace voxel {
namespace world {

constexpr int WORLDGEN_TREES = 1 << 0;
constexpr int WORLDGEN_CLOUDS = 1 << 1;

constexpr int WORLDGEN_CLIENT = WORLDGEN_TREES | WORLDGEN_CLOUDS;
constexpr int WORLDGEN_SERVER = WORLDGEN_TREES;

extern int fillVoxels(int x, int z, const WorldContext& worldCtx, Voxel* voxels, BiomeManager& biomManager, long seed, int noiseSeedOffsetX, int noiseSeedOffsetZ, int maxHeight);

template<class Volume>
static void buildCity(Volume& volume, const Region& region, core::Random& random, const BiomeManager& biomManager) {
	// TODO: apply gradient at city positions and then build houses
	glm::ivec3 buildingPos = region.getCentre();
	if (!biomManager.hasCity(buildingPos)) {
		return;
	}
	for (int i = MAX_TERRAIN_HEIGHT - 1; i >= MAX_WATER_HEIGHT; --i) {
		const VoxelType material = volume.getVoxel(buildingPos.x, i, buildingPos.z).getMaterial();
		if (isFloor(material)) {
			buildingPos.y = i;
			if (random.fithyFifthy()) {
				voxel::building::createBuilding(volume, buildingPos, voxel::BuildingType::House, random);
			} else {
				voxel::building::createBuilding(volume, buildingPos, voxel::BuildingType::Tower, random);
			}
			break;
		}
	}
}

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

	core_assert((lowerZ + depth) % 2 == 0);
	core_assert((lowerX + width) % 2 == 0);
	for (int z = lowerZ; z < lowerZ + depth; z += 2) {
		for (int x = lowerX; x < lowerX + width; x += 2) {
			const int ni = fillVoxels(x, z, worldCtx, voxels, biomManager, seed, noiseSeedOffsetX, noiseSeedOffsetZ, MAX_TERRAIN_HEIGHT - 1);
			volume.setVoxels(x, 0, z, 2, 2, voxels, ni);
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
	buildCity(volume, region, random, biomManager);
}

}
}
