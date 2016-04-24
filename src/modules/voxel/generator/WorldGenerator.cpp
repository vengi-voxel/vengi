#include "WorldGenerator.h"
#include "TreeGenerator.h"
#include "CloudGenerator.h"
#include "core/Var.h"
#include "noise/SimplexNoise.h"

namespace voxel {

void WorldGenerator::createWorld(WorldContext& worldCtx, TerrainContext& ctx, BiomManager& biomManager, core::Random& random, int flags, int noiseSeedOffsetX, int noiseSeedOffsetZ) {
	const Region& region = ctx.region;
	Log::debug("Create new chunk at %i:%i:%i", region.getCentreX(), region.getCentreY(), region.getCentreZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int height = region.getHeightInVoxels();
	const int lowerY = region.getLowerY();
	const int lowerX = region.getLowerX();
	const int lowerZ = region.getLowerZ();
	// TODO: kill me
	const core::VarPtr& plainTerrain = core::Var::get("voxel-plainterrain", "false");
	const bool plainTerrainBool = plainTerrain->boolVal();

	// TODO: the 2d noise doesn't neep the same resolution - we can optimize this a lot
	for (int z = 0; z < depth; ++z) {
		for (int x = 0; x < width; ++x) {
			const glm::vec2 noisePos2d = glm::vec2(noiseSeedOffsetX + lowerX + x, noiseSeedOffsetZ + lowerZ + z);
			const float landscapeNoise = noise::Simplex::Noise2D(noisePos2d, worldCtx.landscapeNoiseOctaves,
					worldCtx.landscapeNoisePersistence, worldCtx.landscapeNoiseFrequency, worldCtx.landscapeNoiseAmplitude);
			const float noiseNormalized = noise::norm(landscapeNoise);
			const float mountainNoise = noise::Simplex::Noise2D(noisePos2d, worldCtx.mountainNoiseOctaves,
					worldCtx.mountainNoisePersistence, worldCtx.mountainNoiseFrequency, worldCtx.mountainNoiseAmplitude);
			const float mountainNoiseNormalized = noise::norm(mountainNoise);
			const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
			const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
			const int ni = n * (MAX_TERRAIN_HEIGHT - 1);
			int y = 0;
			int start = lowerY;
			if (start == y) {
				++start;
				const Voxel& voxel = biomManager.getVoxelType(lowerX + x, 0, lowerZ + z);
				ctx.chunk->setVoxel(x, 0, z, voxel);
			}
			if (plainTerrainBool) {
				for (int h = lowerY; h < ni; ++h) {
					const Voxel& voxel = biomManager.getVoxelType(lowerX + x, h, lowerZ + z);
					ctx.chunk->setVoxel(x, y, z, voxel);
				}
			} else {
				for (int h = lowerY; h < ni; ++h) {
					const glm::vec3 noisePos3d = glm::vec3(noisePos2d.x, h, noisePos2d.y);
					const float noiseVal = noise::norm(
							noise::Simplex::Noise3D(noisePos3d, worldCtx.caveNoiseOctaves, worldCtx.caveNoisePersistence,
									worldCtx.caveNoiseFrequency, worldCtx.caveNoiseAmplitude));
					const float finalDensity = noiseNormalized + noise::norm(noiseVal);
					if (finalDensity > worldCtx.caveDensityThreshold) {
						const Voxel& voxel = biomManager.getVoxelType(lowerX + x, h, lowerZ + z);
						ctx.chunk->setVoxel(x, y, z, voxel);
					}
					if (++y >= height) {
						break;
					}
				}
			}
		}
	}
	const glm::vec3 worldPos(lowerX, lowerY, lowerZ);
	if ((flags & WORLDGEN_CLOUDS) && biomManager.hasClouds(worldPos)) {
		CloudGenerator::createClouds(ctx, random);
	}
	if (biomManager.hasTrees(worldPos)) {
		TreeGenerator::createTrees(ctx, random);
	}

}

}
