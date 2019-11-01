/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace voxelworld {

enum class TreeType : int32_t {
	Dome,
	DomeHangingLeaves,
	Cone,
	Ellipsis,
	BranchesEllipsis,
	Cube,
	CubeSideCubes,
	Pine,
	Fir,
	Palm,
	SpaceColonization,
	Max
};

extern TreeType getTreeType(const char *str);

}
