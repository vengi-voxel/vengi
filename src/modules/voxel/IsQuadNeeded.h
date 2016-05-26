#pragma once

#include "polyvox/Voxel.h"

namespace voxel {

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsQuadNeeded {
private:
	inline bool is(bool negative, const Voxel& v) const {
		const VoxelType m = v.getMaterial();
		if (m == Air) {
			return negative;
		} else if (!_water && m == Water) {
			return negative;
		}
		return !negative;
	}

	bool _water;
public:
	IsQuadNeeded(bool water) :
			_water(water) {
	}

	inline bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse, FaceNames face, int x, int z) const {
		if (_water) {
			if (front.getMaterial() == Air && back.getMaterial() == Water && face == PositiveY) {
				materialToUse = back;
				return true;
			}
			return false;
		}
		if (is(false, back) && is(true, front)) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

}
