/**
 * @file
 */

#include "VolumeCache.h"
#include "animation/chr/CharacterSkeleton.h"
#include "core/io/Filesystem.h"
#include "core/App.h"
#include "core/Log.h"
#include "core/Common.h"
#include "voxelformat/Loader.h"
#include "voxelformat/VoxFileFormat.h"

namespace voxedit {
namespace anim {

bool VolumeCache::load(const core::String& fullPath, size_t volumeIndex, voxel::VoxelVolumes& volumes) {
	Log::info("Loading volume from %s", fullPath.c_str());
	const io::FilesystemPtr& fs = io::filesystem();
	const io::FilePtr& file = fs->open(fullPath);
	voxel::VoxelVolumes localVolumes;
	// TODO: use the cache luke
	if (!voxelformat::loadVolumeFormat(file, localVolumes)) {
		Log::error("Failed to load %s", file->name().c_str());
		return false;
	}
	if ((int)localVolumes.size() != 1) {
		Log::error("More than one volume/layer found in %s", file->name().c_str());
		return false;
	}
	volumes[volumeIndex] = localVolumes[0];
	return true;
}

bool VolumeCache::getVolumes(const animation::AnimationSettings& settings, voxel::VoxelVolumes& volumes) {
	volumes.resize(animation::AnimationSettings::MAX_ENTRIES);

	for (size_t i = 0; i < animation::AnimationSettings::MAX_ENTRIES; ++i) {
		if (settings.paths[i].empty()) {
			continue;
		}
		const core::String& fullPath = settings.fullPath(i);
		if (!load(fullPath, i, volumes)) {
			Log::error("Failed to load %s", settings.paths[i].c_str());
			return false;
		}
	}
	for (int i = 0; i < (int)animation::AnimationSettings::MAX_ENTRIES; ++i) {
		if (volumes[i].volume == nullptr) {
			continue;
		}
		volumes[i].name = settings.meshType(i);
	}
	return true;
}

}
}
