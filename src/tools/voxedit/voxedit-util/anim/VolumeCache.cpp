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

bool VolumeCache::getCharacterVolumes(const animation::CharacterSettings& settings, voxel::VoxelVolumes& volumes) {
	volumes.resize(std::enum_value(animation::CharacterMeshType::Max));

	for (size_t i = 0; i < settings.paths.size(); ++i) {
		if (settings.paths[i] == nullptr || settings.paths[i]->empty()) {
			continue;
		}
		const std::string& fullPath = settings.fullPath((animation::CharacterMeshType)i);
		if (!load(fullPath, (animation::CharacterMeshType)i, volumes)) {
			Log::error("Failed to load %s", settings.paths[i]->c_str());
			return false;
		}
	}
	for (int i = 0; i < std::enum_value(animation::CharacterMeshType::Max); ++i) {
		if (volumes[i].volume == nullptr) {
			continue;
		}
		volumes[i].name = toString((animation::CharacterMeshType)i);
	}
	return true;
}

bool VolumeCache::load(const std::string& fullPath, animation::CharacterMeshType meshType, voxel::VoxelVolumes& volumes) {
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
	volumes[std::enum_value(meshType)] = localVolumes[0];
	return true;
}

}
}
