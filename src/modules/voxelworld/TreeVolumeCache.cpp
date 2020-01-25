/**
 * @file
 */

#include "TreeVolumeCache.h"
#include "core/App.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/io/Filesystem.h"
#include <glm/vec3.hpp>

namespace voxelworld {

TreeVolumeCache::TreeVolumeCache(const voxelformat::VolumeCachePtr& volumeCache) :
		_volumeCache(volumeCache) {
}

bool TreeVolumeCache::init() {
	Log::debug("Initialize the tree volume cache");
	std::vector<io::Filesystem::DirEntry> entities;
	if (!io::filesystem()->list("models/trees/", entities, "*")) {
		Log::error("Failed to list tree models");
		return false;
	}
	Log::debug("Found %i tree types", (int)entities.size());
	for (const auto& e : entities) {
		if (e.type != io::Filesystem::DirEntry::Type::dir) {
			continue;
		}
		std::vector<io::Filesystem::DirEntry> treeFiles;
		const std::string& treeTypeDir = core::string::format("models/trees/%s/", e.name.c_str());
		if (!io::filesystem()->list(treeTypeDir, treeFiles, "*.vox")) {
			Log::warn("Failed to list tree models in %s", treeTypeDir.c_str());
			continue;
		}
		_treeTypeCount.put(e.name, (int)treeFiles.size());
	}
	return true;
}

void TreeVolumeCache::shutdown() {
	_volumeCache = voxelformat::VolumeCachePtr();
	_treeTypeCount.clear();
}

voxel::RawVolume* TreeVolumeCache::loadTree(const glm::ivec3& treePos, const char *treeType) {
	int treeCount = 1;
	if (!_treeTypeCount.get(treeType, treeCount)) {
		Log::warn("Could not get tree type count for %s - assuming 1", treeType);
	}
	if (treeCount <= 0) {
		return nullptr;
	}
	const int treeIndex = 1 + ((treePos.x + treePos.z) % treeCount);
	char filename[64];
	if (!core::string::formatBuf(filename, sizeof(filename), "models/trees/%s/%i.vox", treeType, treeIndex)) {
		Log::error("Failed to assemble tree path");
		return nullptr;
	}
	return _volumeCache->loadVolume(filename);
}

}