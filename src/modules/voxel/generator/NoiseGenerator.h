#pragma once

#include "core/Random.h"
#include "noise/Simplex.h"
#include "noise/Noise.h"
#include "voxel/MaterialColor.h"

namespace voxel {
namespace noise {

enum class NoiseType {
	ridgedMF,

	Max
};

static inline float getNoise(const glm::vec2& pos, int octaves, float lacunarity, float gain, NoiseType type) {
	switch (type) {
	case NoiseType::ridgedMF:
		return ::noise::ridgedMF(pos, octaves, lacunarity, gain);
	default:
		return 0.0f;
	}
}

template<class Volume>
void generate(Volume& volume, int octaves, float lacunarity, float frequency, float gain, NoiseType type, core::Random& random) {
	const Region& region = volume.getRegion();
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int lowerX = region.getLowerX();
	const int lowerY = region.getLowerY();
	const int lowerZ = region.getLowerZ();

	const int noiseSeedOffsetX = random.random(0, 1000);
	const int noiseSeedOffsetZ = random.random(0, 1000);

	const Voxel& grass = createRandomColorVoxel(VoxelType::Grass, random);
	const Voxel& dirt = createRandomColorVoxel(VoxelType::Dirt, random);

	glm::vec2 p(noiseSeedOffsetX + lowerX, noiseSeedOffsetZ + lowerZ);
	for (int x = lowerX; x < lowerX + width; ++x, p.x += frequency) {
		for (int z = lowerZ; z < lowerZ + depth; ++z, p.y += frequency) {
			const float n = getNoise(p, octaves, lacunarity, gain, type);
			const int ni = ::noise::norm(n) * (depth - 1);
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
