/**
 * @file
 */

#pragma once

namespace voxedit {

enum class Action : uint8_t {
	None,
	PlaceVoxel,
	CopyVoxel,
	SelectVoxels,
	DeleteVoxel,
	OverrideVoxel
};

}
