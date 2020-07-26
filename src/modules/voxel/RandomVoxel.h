/**
 * @file
 */

#pragma once

#include "voxel/Voxel.h"
#include "math/Random.h"
#include "MaterialColor.h"

namespace math {
class Random;
}

namespace voxel {

/**
 * @brief Helper class to pick a random colored @c VoxelType
 */
class RandomVoxel {
private:
	const MaterialColorIndices& indices;
	const math::Random& _random;
	const VoxelType _type;
	int _sameCount;
	mutable int _amount = 1;
	mutable uint8_t _currentIndex = 0u;
public:
	/**
	 * @param type The VoxelType to pick the color index for
	 * @param random @c math::Random instance
	 * @param sameCount The amount of Voxel instances that are returned as the same
	 * color before a possible change in the color index is evaluated.
	 */
	RandomVoxel(VoxelType type, const math::Random& random, int sameCount = 3);

	operator Voxel() const;
};

}
