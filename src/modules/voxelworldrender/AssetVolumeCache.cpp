/**
 * @file
 */

#include "AssetVolumeCache.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "voxelformat/VolumeFormat.h"
#include "io/Filesystem.h"
#include <glm/vec3.hpp>
#include <glm/common.hpp>

namespace voxelworldrender {

AssetVolumeCache::AssetVolumeCache(const voxelformat::VolumeCachePtr& volumeCache) :
		_volumeCache(volumeCache) {
}

bool AssetVolumeCache::init() {
	Log::debug("Initialize the asset volume cache");
	for (const char **ext = voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD_LIST; *ext; ++ext) {
		core::DynamicArray<io::Filesystem::DirEntry> files;
		if (!io::filesystem()->list("models/plants/", files, core::string::format("*.%s", *ext))) {
			Log::warn("Failed to list assets from models/plants/");
			break;
		}
		_plantCount += (int)files.size();
	}
	Log::debug("Found %i plants", _plantCount);
	return true;
}

void AssetVolumeCache::shutdown() {
	_volumeCache = voxelformat::VolumeCachePtr();
}

voxel::RawVolume* AssetVolumeCache::loadPlant(const glm::ivec3& pos) {
	if (_plantCount <= 0) {
		return nullptr;
	}
	const int index = 1 + (glm::abs(pos.x + pos.z) % _plantCount);
	char filename[64];
	if (!core::string::formatBuf(filename, sizeof(filename), "models/plants/%i", index)) {
		Log::error("Failed to assemble plant path");
		return nullptr;
	}
	return _volumeCache->loadVolume(filename);
}

}
