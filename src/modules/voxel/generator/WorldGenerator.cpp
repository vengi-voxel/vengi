#include "WorldGenerator.h"
#include "TreeGenerator.h"
#include "CloudGenerator.h"
#include "LSystemGenerator.h"
#include "core/Var.h"
#include "core/Trace.h"
#include "noise/SimplexNoise.h"
#include "voxel/Voxel.h"

namespace voxel {

void WorldGenerator::createWorld(WorldContext& worldCtx, TerrainContext& ctx, BiomManager& biomManager, core::Random& random, int flags, int noiseSeedOffsetX, int noiseSeedOffsetZ) {
	const Region& region = ctx.region;
	Log::debug("Create new chunk at %i:%i:%i", region.getCentreX(), region.getCentreY(), region.getCentreZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int lowerX = region.getLowerX();
	const int lowerZ = region.getLowerZ();
	// TODO: kill me
	const core::VarPtr& plainTerrain = core::Var::get("voxel-plainterrain", "false");
	const bool plainTerrainBool = plainTerrain->boolVal();
	Voxel voxels[MAX_TERRAIN_HEIGHT];

	// TODO: the 2d noise doesn't neep the same resolution - we can optimize this a lot
	for (int z = lowerZ; z < lowerZ + depth; ++z) {
		for (int x = lowerX; x < lowerX + width; ++x) {
			const glm::vec2 noisePos2d(noiseSeedOffsetX + x, noiseSeedOffsetZ + z);
			const float landscapeNoise = noise::Simplex::Noise2D(noisePos2d, worldCtx.landscapeNoiseOctaves,
					worldCtx.landscapeNoisePersistence, worldCtx.landscapeNoiseFrequency, worldCtx.landscapeNoiseAmplitude);
			const float noiseNormalized = noise::norm(landscapeNoise);
			const float mountainNoise = noise::Simplex::Noise2D(noisePos2d, worldCtx.mountainNoiseOctaves,
					worldCtx.mountainNoisePersistence, worldCtx.mountainNoiseFrequency, worldCtx.mountainNoiseAmplitude);
			const float mountainNoiseNormalized = noise::norm(mountainNoise);
			const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
			const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
			const int ni = n * (MAX_TERRAIN_HEIGHT - 1);
			if (plainTerrainBool) {
				for (int y = 0; y < ni; ++y) {
					const Voxel& voxel = biomManager.getVoxelType(x, y, z, false);
					voxels[y] = voxel;
				}
			} else {
				const Voxel& air = createVoxel(Air);
				const Voxel& water = createVoxel(Water);
				voxels[0] = createVoxel(Dirt);
				for (int y = ni - 1; y >= 1; --y) {
					const glm::vec3 noisePos3d(noisePos2d.x, y, noisePos2d.y);
					const float noiseVal = noise::norm(
							noise::Simplex::Noise3D(noisePos3d, worldCtx.caveNoiseOctaves, worldCtx.caveNoisePersistence,
									worldCtx.caveNoiseFrequency, worldCtx.caveNoiseAmplitude));
					const float finalDensity = noiseNormalized + noiseVal;
					if (finalDensity > worldCtx.caveDensityThreshold) {
						const bool cave = y < ni - 1;
						const Voxel& voxel = biomManager.getVoxelType(x, y, z, cave, noiseNormalized);
						voxels[y] = voxel;
					} else {
						if (y <= MAX_WATER_HEIGHT) {
							voxels[y] = water;
						} else {
							voxels[y] = air;
						}
					}
				}
			}
			ctx.setVoxels(x, z, voxels, ni);
		}
	}
	if ((flags & WORLDGEN_CLOUDS) != 0) {
		core_trace_scoped(Clouds);
		CloudGenerator::createClouds(ctx, biomManager, random);
	}
	TreeGenerator::createTrees(ctx, biomManager, random);
}

}
