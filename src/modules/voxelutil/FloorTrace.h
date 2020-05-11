/**
 * @file
 */

#pragma once

#include "voxel/PagedVolume.h"
#include "FloorTraceResult.h"

namespace voxelutil {

extern FloorTraceResult findWalkableFloor(voxel::PagedVolume::Sampler *sampler, const glm::ivec3& position, int maxDistanceUpwards);
extern FloorTraceResult findWalkableFloor(voxel::PagedVolume* volume, const glm::ivec3& position, int maxDistanceUpwards);

}
