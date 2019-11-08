/**
 * @file
 */

#pragma once

#include "math/Random.h"
#include "noise/Noise.h"
#include "noise/Simplex.h"
#include "voxel/MaterialColor.h"

namespace voxelgenerator {
namespace noise {

enum class NoiseType {
	ridgedMF,

	Max
};

static inline float getNoise(const glm::ivec2& pos, int octaves, float lacunarity, float frequency, float gain, NoiseType type) {
	const glm::vec2 fpos(pos.x * frequency, pos.y * frequency);
	switch (type) {
	case NoiseType::ridgedMF:
		return ::noise::ridgedMF(fpos, octaves, lacunarity, gain);
	default:
		return 0.0f;
	}
}

template<class Volume>
int generate(Volume& volume, int octaves, float lacunarity, float frequency, float gain, NoiseType type, math::Random& random) {
	int amount = 0;
	const voxel::Region& region = volume.region();
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int height = region.getHeightInVoxels();
	const int lowerX = region.getLowerX();
	const int lowerY = region.getLowerY();
	const int lowerZ = region.getLowerZ();

	const int noiseSeedOffsetX = random.random(0, 1000);
	const int noiseSeedOffsetZ = random.random(0, 1000);

	const voxel::Voxel& grass = voxel::createRandomColorVoxel(voxel::VoxelType::Grass, random);
	const voxel::Voxel& dirt = voxel::createRandomColorVoxel(voxel::VoxelType::Dirt, random);

	for (int x = lowerX; x < lowerX + width; ++x) {
		for (int z = lowerZ; z < lowerZ + depth; ++z) {
			glm::vec2 p(noiseSeedOffsetX + x, noiseSeedOffsetZ + z);
			const float n = getNoise(p, octaves, lacunarity, frequency, gain, type);
			const int ni = ::noise::norm(n) * (height - 1);
			glm::ivec3 vp(x, lowerY, z);
			for (int y = 0; y < ni - 1; ++y) {
				vp.y = lowerY + y;
				if (volume.setVoxel(vp, dirt)) {
					++amount;
				}
			}
			vp.y = ni - 1;
			if (volume.setVoxel(vp, grass)) {
				++amount;
			}
		}
	}
	return amount;
}

}
}
