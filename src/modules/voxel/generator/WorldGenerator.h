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

	glm::ivec3 pos(glm::uninitialize);

	const Voxel& water = createColorVoxel(VoxelType::Water, seed);
	const Voxel& dirt = createColorVoxel(VoxelType::Dirt, seed);
	static constexpr Voxel air;

	// TODO: the 2d noise doesn't neep the same resolution - we can optimize this a lot, we can lerp/glm::mix here
	for (int z = lowerZ; z < lowerZ + depth; ++z) {
		pos.z = z;
		for (int x = lowerX; x < lowerX + width; ++x) {
			pos.x = x;
			const glm::vec2 noisePos2d(noiseSeedOffsetX + x, noiseSeedOffsetZ + z);
			const float landscapeNoise = ::noise::Simplex::Noise2D(noisePos2d, worldCtx.landscapeNoiseOctaves,
					worldCtx.landscapeNoisePersistence, worldCtx.landscapeNoiseFrequency, worldCtx.landscapeNoiseAmplitude);
			const float noiseNormalized = ::noise::norm(landscapeNoise);
			const float mountainNoise = ::noise::Simplex::Noise2D(noisePos2d, worldCtx.mountainNoiseOctaves,
			worldCtx.mountainNoisePersistence, worldCtx.mountainNoiseFrequency, worldCtx.mountainNoiseAmplitude);
			const float mountainNoiseNormalized = ::noise::norm(mountainNoise);
			const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
			const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
			const int ni = n * (MAX_TERRAIN_HEIGHT - 1);
			voxels[0] = dirt;
			for (int y = ni - 1; y >= 1; --y) {
				pos.y = y;
				const glm::vec3 noisePos3d(noisePos2d.x, y, noisePos2d.y);
				const float noiseVal = ::noise::norm(
						::noise::Simplex::Noise3D(noisePos3d, worldCtx.caveNoiseOctaves, worldCtx.caveNoisePersistence,
								worldCtx.caveNoiseFrequency, worldCtx.caveNoiseAmplitude));
				const float finalDensity = noiseNormalized + noiseVal;
				if (finalDensity > worldCtx.caveDensityThreshold) {
					const bool cave = y < ni - 1;
					const Voxel& voxel = biomManager.getVoxel(pos, cave);
					voxels[y] = voxel;
				} else {
					if (y <= MAX_WATER_HEIGHT) {
						voxels[y] = water;
					} else {
						voxels[y] = air;
					}
				}
			}
			volume.setVoxels(x, z, voxels, ni);
		}
	}
	if ((flags & WORLDGEN_CLOUDS) != 0) {
		core_trace_scoped(Clouds);
		voxel::cloud::createClouds(volume, biomManager, random);
	}
	if ((flags & WORLDGEN_TREES) != 0) {
		core_trace_scoped(Trees);
		voxel::tree::createTrees(volume, region, biomManager, random);
	}
}

}
}
