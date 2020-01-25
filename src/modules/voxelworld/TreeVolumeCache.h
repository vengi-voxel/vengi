/**
 * @file
 */

#include "voxelformat/VolumeCache.h"
#include "core/collection/Map.h"
#include <glm/fwd.hpp>
#include <string>

namespace voxelworld {

class TreeVolumeCache {
private:
	core::Map<std::string, int, 8, std::hash<std::string>> _treeTypeCount;

	voxelformat::VolumeCachePtr _volumeCache;
public:
	TreeVolumeCache(const voxelformat::VolumeCachePtr& volumeCache);

	bool init();
	void shutdown();
	voxel::RawVolume* loadTree(const glm::ivec3& treePos, const char *treeType);
};

}