/**
 * @file
 */

#pragma once

#include "math/Random.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxelgenerator {
namespace noise {

enum class NoiseType {
	ridgedMF,

	Max
};

extern int generate(voxel::RawVolumeWrapper& volume, int octaves, float lacunarity, float frequency, float gain, NoiseType type, math::Random& random);

}
}
