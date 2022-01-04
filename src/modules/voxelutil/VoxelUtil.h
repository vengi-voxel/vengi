/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxelutil {

extern bool copyIntoRegion(const voxel::RawVolume &in, voxel::RawVolume &out, const voxel::Region &targetRegion);
extern bool copy(const voxel::RawVolume &in, const voxel::Region& inRegion, voxel::RawVolume &out, const voxel::Region &outRegion);
/**
 * @brief Checks whether the given region of the volume is only filled with air
 * @return @c true if no blocking voxel is inside the region, @c false otherwise
 * @sa voxel::isBlocked()
 */
extern bool isEmpty(const voxel::RawVolume &in, const voxel::Region &region);

}
