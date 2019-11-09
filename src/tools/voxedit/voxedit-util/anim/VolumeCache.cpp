/**
 * @file
 */

#include "VolumeCache.h"
#include "animation/Skeleton.h"
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
		if (settings.paths[i] == nullptr) {
			continue;
		}
		if (!load(settings.basePath, settings.paths[i]->c_str(), (animation::CharacterMeshType)i, volumes)) {
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

bool VolumeCache::load(const char *basePath, const char *filename, animation::CharacterMeshType meshType, voxel::VoxelVolumes& volumes) {
	if (filename == nullptr || filename[0] == '\0') {
		Log::error("No filename given - can't load character mesh");
		return false;
	}
	char fullPath[128];
	if (!core::string::formatBuf(fullPath, sizeof(fullPath), "%s/%s.vox", basePath, filename)) {
		Log::error("Failed to initialize the character path buffer. Can't load %s.", fullPath);
		return false;
	}
	Log::info("Loading volume from %s", fullPath);
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
