/**
 * @file
 */

#pragma once

#include "voxelformat/VolumeCache.h"
#include "core/collection/StringMap.h"
#include <glm/fwd.hpp>

namespace voxelworld {

class TreeVolumeCache {
private:
	core::StringMap<int> _treeTypeCount;

	voxelformat::VolumeCachePtr _volumeCache;
public:
	TreeVolumeCache(const voxelformat::VolumeCachePtr& volumeCache);

	bool init();
	void shutdown();
	voxel::RawVolume* loadTree(const glm::ivec3& treePos, const char *treeType);
};

}