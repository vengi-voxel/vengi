/**
 * @file
 */

#pragma once

#include "WorldMgr.h"
#include <glm/vec4.hpp>

namespace voxelworld {

class CachedFloorResolver {
private:
	glm::ivec3 _lastPos { -1 };
	int _lastMaxDistanceY = -1;
	int _lastY = -1;
	voxel::PagedVolume::Sampler* _sampler = nullptr;
	voxelworld::WorldMgrPtr _worldMgr;
public:
	int findWalkableFloor(const glm::ivec3& position, int maxDistanceY);
	bool init(const voxelworld::WorldMgrPtr& worldMgr);
	void shutdown();
};

}