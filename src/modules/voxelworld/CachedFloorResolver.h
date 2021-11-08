/**
 * @file
 */

#pragma once

#include "WorldMgr.h"
#include "voxelutil/FloorTraceResult.h"
#include <glm/vec4.hpp>

namespace voxelworld {

class CachedFloorResolver {
private:
	glm::ivec3 _lastPos { -1 };
	int _lastMaxDistanceY = -1;
	voxelutil::FloorTraceResult _last;
public:
	voxelutil::FloorTraceResult findWalkableFloor(voxel::PagedVolume::Sampler* sampler, const glm::ivec3& position, int maxDistanceY);
};

}
