/**
 * @file
 */

#include "CachedFloorResolver.h"
#include "voxelutil/FloorTrace.h"

namespace voxelworld {

voxelutil::FloorTraceResult CachedFloorResolver::findWalkableFloor(const glm::ivec3& position, int maxDistanceY) {
	if (_lastPos == position && _lastMaxDistanceY == maxDistanceY) {
		return _last;
	}
	voxelutil::FloorTraceResult trace = voxelutil::findWalkableFloor(_sampler, position, maxDistanceY);
	_lastPos = position;
	_lastMaxDistanceY = maxDistanceY;
	_last = trace;
	return trace;
}

bool CachedFloorResolver::init(const voxelworld::WorldMgrPtr& worldMgr) {
	_worldMgr = worldMgr;
	_sampler = new voxel::PagedVolume::Sampler(_worldMgr->volumeData());
	return true;
}

void CachedFloorResolver::shutdown() {
	delete _sampler;
}

}