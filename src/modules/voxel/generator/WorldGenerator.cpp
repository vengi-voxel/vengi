#include "WorldGenerator.h"
#include "noise/Noise.h"

namespace voxel {
namespace world {

WorldGenerator::WorldGenerator(BiomeManager& biomeManager, long seed) :
		_biomeManager(biomeManager), _seed(seed), _random(seed) {
}

float WorldGenerator::getHeight(const glm::vec2& noisePos2d, const WorldContext& worldCtx) const {
	// TODO: move the noise settings into the biome
	const float landscapeNoise = _noise.fbmNoise2D(noisePos2d, worldCtx.landscapeNoiseOctaves,
			worldCtx.landscapeNoisePersistence, worldCtx.landscapeNoiseFrequency, worldCtx.landscapeNoiseAmplitude);
	const float noiseNormalized = ::noise::norm(landscapeNoise);
	const float mountainNoise = _noise.fbmNoise2D(noisePos2d, worldCtx.mountainNoiseOctaves,
			worldCtx.mountainNoisePersistence, worldCtx.mountainNoiseFrequency, worldCtx.mountainNoiseAmplitude);
	const float mountainNoiseNormalized = ::noise::norm(mountainNoise);
	const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
	const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
	return n;
}

int WorldGenerator::fillVoxels(int x, int lowerY, int z, const WorldContext& worldCtx, Voxel* voxels, int noiseSeedOffsetX, int noiseSeedOffsetZ, int maxHeight) const {
	const glm::vec2 noisePos2d(noiseSeedOffsetX + x, noiseSeedOffsetZ + z);
	const float n = getHeight(noisePos2d, worldCtx);
	const glm::ivec3 noisePos3d(x, lowerY, z);
	int centerHeight;
	const float cityMultiplier = _biomeManager.getCityMultiplier(glm::ivec2(x, z), &centerHeight);
	int ni = n * maxHeight;
	if (cityMultiplier < 1.0f) {
		const float revn = (1.0f - cityMultiplier);
		ni = revn * centerHeight + (n * maxHeight * cityMultiplier);
	}
	if (ni < lowerY) {
		return 0;
	}

	const Voxel& water = createColorVoxel(VoxelType::Water, _seed);
	const Voxel& dirt = createColorVoxel(VoxelType::Dirt, _seed);
	static constexpr Voxel air;

	voxels[0] = dirt;
	glm::ivec3 pos(x, 0, z);
	for (int y = ni - 1; y >= lowerY + 1; --y) {
		const glm::vec3 noisePos3d(noisePos2d.x, y, noisePos2d.y);
		// TODO: move the noise settings into the biome
		const float noiseVal = ::noise::norm(
				_noise.fbmNoise3D(noisePos3d, worldCtx.caveNoiseOctaves, worldCtx.caveNoisePersistence,
						worldCtx.caveNoiseFrequency, worldCtx.caveNoiseAmplitude));
		const float finalDensity = n + noiseVal;
		if (finalDensity > worldCtx.caveDensityThreshold) {
			const bool cave = y < ni - 1;
			pos.y = y;
			const Voxel& voxel = _biomeManager.getVoxel(pos, cave);
			voxels[y] = voxel;
		} else {
			if (y < MAX_WATER_HEIGHT) {
				voxels[y] = water;
			} else {
				voxels[y] = air;
			}
		}
	}
	for (int i = lowerY; i < MAX_WATER_HEIGHT; ++i) {
		if (voxels[i] == air) {
			voxels[i] = water;
		}
	}
	return std::max(ni - lowerY, MAX_WATER_HEIGHT - lowerY);
}

}
}
