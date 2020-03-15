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

	/**
	 * @brief Ensure that the same volume is returned for the same input parameters. But still
	 * hand out random trees for the given type.
	 * @param[in] treePos world position
	 * @param[in] treeType the type is used to fill the path below @c models/trees - also check
	 * the registered biome tree types
	 * @return voxel::RawVolume or @c nullptr if no tree volume was found for the given tree type.
	 */
	voxel::RawVolume* loadTree(const glm::ivec3& treePos, const char *treeType);
};

}