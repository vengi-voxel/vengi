/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"

namespace voxelformat {
namespace priv {

enum class SLABVisibility : uint8_t { None = 0, Left = 1, Right = 2, Front = 4, Back = 8, Up = 16, Down = 32 };
CORE_ENUM_BIT_OPERATIONS(SLABVisibility)

inline SLABVisibility calculateVisibility(const voxel::RawVolume *v, int x, int y, int z) {
	SLABVisibility vis = SLABVisibility::None;
	voxel::FaceBits visBits = voxel::visibleFaces(*v, x, y, z);
	if (visBits == voxel::FaceBits::None) {
		return vis;
	}
	// x
	if ((visBits & voxel::FaceBits::NegativeX) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Left;
	}
	if ((visBits & voxel::FaceBits::PositiveX) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Right;
	}
	// y (our z)
	if ((visBits & voxel::FaceBits::NegativeZ) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Front;
	}
	if ((visBits & voxel::FaceBits::PositiveZ) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Back;
	}
	// z (our y) is running from top to bottom
	if ((visBits & voxel::FaceBits::NegativeY) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Up;
	}
	if ((visBits & voxel::FaceBits::PositiveY) != voxel::FaceBits::None) {
		vis |= SLABVisibility::Down;
	}
	return vis;
}

} // namespace priv
} // namespace voxelformat
