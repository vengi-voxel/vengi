/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxelutil {

extern bool copyIntoRegion(const voxel::RawVolume &in, voxel::RawVolume &out, const voxel::Region &targetRegion);
extern bool copy(const voxel::RawVolume &in, const voxel::Region& inRegion, voxel::RawVolume &out, const voxel::Region &outRegion);

}
