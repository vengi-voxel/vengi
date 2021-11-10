/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxelutil {

extern void copyIntoRegion(const voxel::RawVolume &in, voxel::RawVolume &out, const voxel::Region &targetRegion);

}
