#include "WorldGenerator.h"

namespace voxel {
namespace world {

// http://www.gamasutra.com/blogs/MattKlingensmith/20130811/198049/How_we_Generate_Terrain_in_DwarfCorp.php?print=1
// http://hss.ulb.uni-bonn.de/2013/3124/3124-engl.htm
// https://en.wikipedia.org/wiki/Gradient_descent (Erosion)
int fillVoxels(int x, int z, const WorldContext& worldCtx, Voxel* voxels, BiomeManager& biomManager, long seed, int noiseSeedOffsetX, int noiseSeedOffsetZ, int maxHeight) {
	// TODO: the 2d noise doesn't need the same resolution - we can optimize this a lot, we can lerp/glm::mix here
	const glm::vec2 noisePos2d(noiseSeedOffsetX + x, noiseSeedOffsetZ + z);
	const float landscapeNoise = ::noise::Simplex::Noise2D(noisePos2d, worldCtx.landscapeNoiseOctaves,
			worldCtx.landscapeNoisePersistence, worldCtx.landscapeNoiseFrequency, worldCtx.landscapeNoiseAmplitude);
	const float noiseNormalized = ::noise::norm(landscapeNoise);
	const float mountainNoise = ::noise::Simplex::Noise2D(noisePos2d, worldCtx.mountainNoiseOctaves,
			worldCtx.mountainNoisePersistence, worldCtx.mountainNoiseFrequency, worldCtx.mountainNoiseAmplitude);
	const float mountainNoiseNormalized = ::noise::norm(mountainNoise);
	const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
	const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
	const int ni = n * maxHeight;

	const Voxel& water = createColorVoxel(VoxelType::Water, seed);
	const Voxel& dirt = createColorVoxel(VoxelType::Dirt, seed);
	static constexpr Voxel air;

	// TODO: apply city gradient from biome manager

	voxels[0] = dirt;
	for (int y = ni - 1; y >= 1; --y) {
		const glm::vec3 noisePos3d(noisePos2d.x, y, noisePos2d.y);
		const float noiseVal = ::noise::norm(
				::noise::Simplex::Noise3D(noisePos3d, worldCtx.caveNoiseOctaves, worldCtx.caveNoisePersistence,
						worldCtx.caveNoiseFrequency, worldCtx.caveNoiseAmplitude));
		const float finalDensity = n + noiseVal;
		if (finalDensity > worldCtx.caveDensityThreshold) {
			const bool cave = y < ni - 1;
			const glm::ivec3 pos(x, y, z);
			const Voxel& voxel = biomManager.getVoxel(pos, cave);
			voxels[y] = voxel;
		} else {
			if (y < MAX_WATER_HEIGHT) {
				voxels[y] = water;
			} else {
				voxels[y] = air;
			}
		}
	}
	for (int i = 0; i < MAX_WATER_HEIGHT; ++i) {
		if (voxels[i] == air) {
			voxels[i] = water;
		}
	}
	return std::max(ni, MAX_WATER_HEIGHT);
}

}
}
