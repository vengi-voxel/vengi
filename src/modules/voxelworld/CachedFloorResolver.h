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
	voxel::PagedVolume::Sampler* _sampler = nullptr;
	voxelworld::WorldMgrPtr _worldMgr;
public:
	voxelutil::FloorTraceResult findWalkableFloor(const glm::ivec3& position, int maxDistanceY);
	bool init(const voxelworld::WorldMgrPtr& worldMgr);
	void shutdown();
};

}