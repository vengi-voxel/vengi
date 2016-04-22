#pragma once

#include "Voxel.h"
#include <cstdint>

namespace voxel {

/// Default implementation of a function object for deciding when
/// the cubic surface extractor should insert a face between two voxels.
///
/// The criteria used here are that the voxel in front of the potential
/// quad should have a value of zero (which would typically indicate empty
/// space) while the voxel behind the potential quad would have a value
/// geater than zero (typically indicating it is solid). Note that for
/// different behaviour users can create their own implementation and pass
/// it to extractCubicMesh().
class DefaultIsQuadNeeded {
public:
	bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse) {
		if (back.getMaterial() != Air && front.getMaterial() == Air) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

}
