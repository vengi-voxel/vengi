#pragma once

#include "math/Random.h"
#include "noise/Noise.h"
#include "voxel/MaterialColor.h"

namespace voxel {
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
void generate(Volume& volume, int octaves, float lacunarity, float frequency, float gain, NoiseType type, core::Random& random) {
	const Region& region = volume.region();
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int height = region.getHeightInVoxels();
	const int lowerX = region.getLowerX();
	const int lowerY = region.getLowerY();
	const int lowerZ = region.getLowerZ();

	const int noiseSeedOffsetX = random.random(0, 1000);
	const int noiseSeedOffsetZ = random.random(0, 1000);

	const Voxel& grass = createRandomColorVoxel(VoxelType::Grass, random);
	const Voxel& dirt = createRandomColorVoxel(VoxelType::Dirt, random);

	for (int x = lowerX; x < lowerX + width; ++x) {
		for (int z = lowerZ; z < lowerZ + depth; ++z) {
			glm::vec2 p(noiseSeedOffsetX + x, noiseSeedOffsetZ + z);
			const float n = getNoise(p, octaves, lacunarity, frequency, gain, type);
			const int ni = ::noise::norm(n) * (height - 1);
			glm::ivec3 vp(x, lowerY, z);
			if (ni > 0) {
				volume.setVoxel(vp, dirt);
			}
			for (int y = 1; y < ni; ++y) {
				vp.y = lowerY + y;
				volume.setVoxel(vp, grass);
			}
		}
	}
}

}
}
