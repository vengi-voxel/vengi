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

}
}
