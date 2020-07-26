/**
 * @file
 */

#include "RandomVoxel.h"
#include "core/Assert.h"
#include "math/Random.h"

namespace voxel {

RandomVoxel::RandomVoxel(VoxelType type, const math::Random& random, int sameCount) :
		indices(getMaterialIndices(type)), _random(random), _type(type), _sameCount(sameCount) {
	core_assert(!indices.empty());
}

RandomVoxel::operator Voxel() const {
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

}
