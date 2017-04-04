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
#include "voxel/Constants.h"
#include "voxel/WorldContext.h"
#include "voxel/MaterialColor.h"

namespace voxel {
namespace world {

constexpr int WORLDGEN_TREES = 1 << 0;
constexpr int WORLDGEN_CLOUDS = 1 << 1;

constexpr int WORLDGEN_CLIENT = WORLDGEN_TREES | WORLDGEN_CLOUDS;
constexpr int WORLDGEN_SERVER = WORLDGEN_TREES;

class WorldGenerator {
private:
	BiomeManager& _biomManager;
	long _seed;
	core::Random _random;

	int fillVoxels(int x, int y, int z, const WorldContext& worldCtx, Voxel* voxels, int noiseSeedOffsetX, int noiseSeedOffsetZ, int maxHeight) const;
	float getHeight(const glm::vec2& noisePos2d, const WorldContext& worldCtx) const;
public:
	WorldGenerator(BiomeManager& biomManager, long seed = 0);

	template<class Volume>
	bool createBuildings(Volume& volume) {
		// TODO: apply gradient at city positions and then build houses
		const voxel::Region& region = volume.getRegion();
		glm::ivec3 buildingPos = region.getCentre();
		if (!_biomManager.hasCity(buildingPos)) {
			return false;
		}
		for (int i = MAX_TERRAIN_HEIGHT - 1; i >= MAX_WATER_HEIGHT; --i) {
			const VoxelType material = volume.getVoxel(buildingPos.x, i, buildingPos.z).getMaterial();
			if (isFloor(material)) {
				buildingPos.y = i;
				if (_random.fithyFifthy()) {
					voxel::building::createBuilding(volume, buildingPos, voxel::BuildingType::House, _random);
				} else {
					voxel::building::createBuilding(volume, buildingPos, voxel::BuildingType::Tower, _random);
				}
				break;
			}
		}
		return true;
	}

	template<class Volume>
	void createWorld(const WorldContext& worldCtx, Volume& volume, int noiseSeedOffsetX, int noiseSeedOffsetZ) const {
		core_trace_scoped(WorldGeneration);
		const Region& region = volume.getRegion();
		Log::debug("Create new chunk at %i:%i:%i", region.getLowerX(), region.getLowerY(), region.getLowerZ());
		const int width = region.getWidthInVoxels();
		const int depth = region.getDepthInVoxels();
		const int lowerX = region.getLowerX();
		const int lowerY = region.getLowerY();
		const int lowerZ = region.getLowerZ();
		core_assert(region.getLowerY() >= 0);
		Voxel voxels[MAX_TERRAIN_HEIGHT];

		const int size = 2;
		core_assert(depth % size == 0);
		core_assert(width % size == 0);
		for (int z = lowerZ; z < lowerZ + depth; z += size) {
			for (int x = lowerX; x < lowerX + width; x += size) {
				const int ni = fillVoxels(x, lowerY, z, worldCtx, voxels, noiseSeedOffsetX, noiseSeedOffsetZ, MAX_TERRAIN_HEIGHT - 1);
				volume.setVoxels(x, lowerY, z, size, size, voxels, ni);
			}
		}
	}

	template<class Volume>
	bool createClouds(Volume& volume, voxel::cloud::CloudContext& ctx) {
		core_trace_scoped(Clouds);
		const voxel::Region& region = volume.getRegion();
		return voxel::cloud::createClouds(volume, region, _biomManager, ctx, _random);
	}

	template<class Volume>
	void createTrees(Volume& volume) {
		core_trace_scoped(Trees);
		const voxel::Region& region = volume.getRegion();
		voxel::tree::createTrees(volume, region, _biomManager, _random);
	}
};

}
}
