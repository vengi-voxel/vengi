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
		if (!isBlocked(m) || isWater(m)) {
			return negative;
		}
		return !negative;
	}

public:
	inline bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse, FaceNames face, int x, int z) const {
		if (is(false, back) && is(true, front)) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsWaterQuadNeeded {
	inline bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse, FaceNames face, int x, int z) const {
		if (face == PositiveY && !isBlocked(front.getMaterial()) && isWater(back.getMaterial())) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

}
