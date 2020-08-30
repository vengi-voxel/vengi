/**
 * @file
 */

#include "TreeVolumeCache.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "voxelformat/VolumeFormat.h"
#include "core/io/Filesystem.h"
#include <glm/vec3.hpp>
#include <glm/common.hpp>

namespace voxelworld {

TreeVolumeCache::TreeVolumeCache(const voxelformat::VolumeCachePtr& volumeCache) :
		_volumeCache(volumeCache) {
}

bool TreeVolumeCache::init() {
	if (!_treeTypeCount.empty()) {
		return true;
	}
	Log::debug("Initialize the tree volume cache");
	core::DynamicArray<io::Filesystem::DirEntry> entities;
	if (!io::filesystem()->list("models/trees/", entities, "*")) {
		Log::error("Failed to list tree models");
		return false;
	}
	Log::debug("Found %i tree types", (int)entities.size());
	for (const auto& e : entities) {
		if (e.type != io::Filesystem::DirEntry::Type::dir) {
			continue;
		}
		int amount = 0;
		const core::String& treeTypeDir = core::string::format("models/trees/%s/", e.name.c_str());
		for (const char **ext = voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD_LIST; *ext; ++ext) {
			core::DynamicArray<io::Filesystem::DirEntry> treeFiles;
			if (!io::filesystem()->list(treeTypeDir, treeFiles, core::string::format("*.%s", *ext))) {
				Log::warn("Failed to list tree models in %s", treeTypeDir.c_str());
				break;
			}
			amount += (int)treeFiles.size();
		}
		_treeTypeCount.put(e.name, amount);
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
	const int treeIndex = 1 + (glm::abs(treePos.x + treePos.z) % treeCount);
	char filename[64];
	if (!core::string::formatBuf(filename, sizeof(filename), "models/trees/%s/%i", treeType, treeIndex)) {
		Log::error("Failed to assemble tree path");
		return nullptr;
	}
	return _volumeCache->loadVolume(filename);
}

}
