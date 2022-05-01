/**
 * @file
 */

#pragma once

#include "voxel/Face.h"
#include "voxel/RawVolume.h"

namespace voxelutil {

bool copyIntoRegion(const voxel::RawVolume &in, voxel::RawVolume &out, const voxel::Region &targetRegion);
bool copy(const voxel::RawVolume &in, const voxel::Region& inRegion, voxel::RawVolume &out, const voxel::Region &outRegion);
/**
 * @brief Checks whether the given region of the volume is only filled with air
 * @return @c true if no blocking voxel is inside the region, @c false otherwise
 * @sa voxel::isBlocked()
 */
bool isEmpty(const voxel::RawVolume &in, const voxel::Region &region);
void fillPlane(voxel::RawVolume &in, const voxel::Voxel &voxel, const glm::ivec3 &position, voxel::FaceNames face);
void fillHollow(voxel::RawVolume &in, const voxel::Voxel &voxel);

}
