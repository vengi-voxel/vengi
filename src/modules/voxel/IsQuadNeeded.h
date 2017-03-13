#pragma once

#include "polyvox/Voxel.h"

namespace voxel {

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsQuadNeeded {
	inline bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse, FaceNames face) const {
		const VoxelType backMaterial = back.getMaterial();
		if (isAir(backMaterial) || isWater(backMaterial)) {
			return false;
		}
		const VoxelType frontMaterial = front.getMaterial();
		if (!isAir(frontMaterial) && !isWater(frontMaterial)) {
			return false;
		}
		materialToUse = back;
		return true;
	}
};

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsWaterQuadNeeded {
	inline bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse, FaceNames face) const {
		if (face == PositiveY && !isBlocked(front.getMaterial()) && isWater(back.getMaterial())) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

}
