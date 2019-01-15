/**
 * @file
 */

#pragma once

namespace voxedit {

enum class Action : uint8_t {
	None,
	PlaceVoxel,
	PlaceVoxels,
	CopyVoxel,
	SelectVoxels,
	DeleteVoxel,
	OverrideVoxel
};

}
