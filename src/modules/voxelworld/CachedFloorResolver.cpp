/**
 * @file
 */

#include "CachedFloorResolver.h"
#include "voxelutil/FloorTrace.h"

namespace voxelworld {

voxelutil::FloorTraceResult CachedFloorResolver::findWalkableFloor(voxel::PagedVolume::Sampler* sampler, const glm::ivec3& position, int maxDistanceY) {
	if (_lastPos == position && _lastMaxDistanceY == maxDistanceY) {
		return _last;
	}
	voxelutil::FloorTraceResult trace = voxelutil::findWalkableFloor(sampler, position, maxDistanceY);
	_lastPos = position;
	_lastMaxDistanceY = maxDistanceY;
	_last = trace;
	return trace;
}

}
