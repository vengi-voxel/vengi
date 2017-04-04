#pragma once

#include "voxel/polyvox/Voxel.h"
#include "core/Random.h"
#include "MaterialColor.h"

namespace voxel {

/**
 * @brief Helper class to pick a random colored @c VoxelType
 */
class RandomVoxel {
private:
	const MaterialColorIndices& indices;
	const core::Random& _random;
	const VoxelType _type;
	int _sameCount;
	mutable int _amount = 1;
	mutable uint8_t _currentIndex = 0u;
public:
	/**
	 * @param type The VoxelType to pick the color index for
	 * @param random @c core::Random instance
	 * @param sameCount The amount of Voxel instances that are returned as the same
	 * color before a possible change in the color index is evaluated.
	 */
	RandomVoxel(VoxelType type, const core::Random& random, int sameCount = 3) :
			indices(getMaterialIndices(type)), _random(random), _type(type), _sameCount(sameCount) {
	}

	inline operator Voxel() const {
		if (indices.size() == 1) {
			return Voxel(_type, indices.front());
		}
		if (_amount == 1) {
			auto i = _random.randomElement(indices.begin(), indices.end());
			_currentIndex = *i;
		}
		++_amount;
		if (_amount >= _sameCount) {
			_amount = 1;
		}
		return Voxel(_type, _currentIndex);
	}
};

}
