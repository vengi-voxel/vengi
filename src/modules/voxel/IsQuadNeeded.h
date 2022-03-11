/**
 * @file
 */

#pragma once

#include "Face.h"
#include "Voxel.h"

namespace voxel {

/**
 * @brief implementation of a function object for deciding when
 * the cubic surface extractor should insert a face between two voxels.
 *
 * The criteria used here are that the voxel in front of the potential
 * quad should have a value of zero (which would typically indicate empty
 * space) while the voxel behind the potential quad would have a value
 * greater than zero (typically indicating it is solid).
 */
struct IsQuadNeeded {
	inline bool operator()(const VoxelType& back, const VoxelType& front, FaceNames face) const {
		if (isAir(back) || isTransparent(back)) {
			return false;
		}
		if (!isAir(front) && !isTransparent(front)) {
			return false;
		}
		return true;
	}
};

}
