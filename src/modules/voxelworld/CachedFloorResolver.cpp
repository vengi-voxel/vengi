/**
 * @file
 */

#include "CachedFloorResolver.h"

namespace voxelworld {

int CachedFloorResolver::findWalkableFloor(const glm::ivec3& position, int maxDistanceY) {
	if (_lastPos == position && _lastMaxDistanceY == maxDistanceY) {
		return _lastY;
	}
	const int y = _worldMgr->findWalkableFloor(_sampler, position, maxDistanceY);
	_lastPos = position;
	_lastMaxDistanceY = maxDistanceY;
	_lastY = y;
	return y;
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