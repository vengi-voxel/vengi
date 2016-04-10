#pragma once

#include <cstdint>

namespace PolyVox {

/// Default implementation of a function object for deciding when
/// the cubic surface extractor should insert a face between two voxels.
///
/// The criteria used here are that the voxel in front of the potential
/// quad should have a value of zero (which would typically indicate empty
/// space) while the voxel behind the potential quad would have a value
/// geater than zero (typically indicating it is solid). Note that for
/// different behaviour users can create their own implementation and pass
/// it to extractCubicMesh().
template<typename VoxelType>
class DefaultIsQuadNeeded {
public:
	bool operator()(VoxelType back, VoxelType front, VoxelType& materialToUse) {
		if (back > 0 && front == 0) {
			materialToUse = static_cast<VoxelType>(back);
			return true;
		}
		return false;
	}
};

}
