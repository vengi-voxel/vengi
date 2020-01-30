/**
 * @file
 */

#pragma once

#include "voxelformat/VolumeCache.h"
#include "core/collection/Map.h"
#include <glm/fwd.hpp>
#include "core/String.h"

namespace voxelworld {

class TreeVolumeCache {
private:
	core::Map<core::String, int, 8, std::hash<core::String>> _treeTypeCount;

	voxelformat::VolumeCachePtr _volumeCache;
public:
	TreeVolumeCache(const voxelformat::VolumeCachePtr& volumeCache);

	bool init();
	void shutdown();
	voxel::RawVolume* loadTree(const glm::ivec3& treePos, const char *treeType);
};

}