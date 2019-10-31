#pragma once

#include "Voxel.h"

namespace voxel {

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsQuadNeeded {
	inline bool operator()(const VoxelType& back, const VoxelType& front, FaceNames face) const {
		if (isAir(back) || isWater(back)) {
			return false;
		}
		if (!isAir(front) && !isWater(front)) {
			return false;
		}
		return true;
	}
};

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsWaterQuadNeeded {
	inline bool operator()(const VoxelType& back, const VoxelType& front, FaceNames face) const {
		if (!isBlocked(front) && isWater(back)) {
			return true;
		}
		return false;
	}
};

}
