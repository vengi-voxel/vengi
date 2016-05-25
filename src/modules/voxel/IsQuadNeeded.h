#pragma once

#include "Voxel.h"

namespace voxel {

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsQuadNeeded {
public:
	inline bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse, FaceNames face, int x, int z) const {
		if (back.getMaterial() != Air && front.getMaterial() == Air) {
			materialToUse = back;
			return true;
		}
		if (back.getMaterial() != Water && front.getMaterial() == Water) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

}
