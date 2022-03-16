/**
 * @file
 */

#include "VolumeCache.h"
#include "animation/chr/CharacterSkeleton.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/StringUtil.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/Format.h"

namespace voxedit {
namespace anim {

bool VolumeCache::load(const core::String& fullPath, int volumeIndex, voxelformat::SceneGraph& sceneGraph, const core::String &name) {
	Log::info("Loading volume from %s into the cache", fullPath.c_str());
	const io::FilesystemPtr& fs = io::filesystem();

	io::FilePtr file;
	for (const char **ext = voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD_LIST; *ext; ++ext) {
		file = fs->open(core::string::format("%s.%s", fullPath.c_str(), *ext));
		if (file->exists()) {
			break;
		}
	}
	if (!file->exists()) {
		Log::error("Failed to load %s for any of the supported format extensions", fullPath.c_str());
		return false;
	}
	voxelformat::SceneGraph newSceneGraph;
	// TODO: use the cache luke
	io::FileStream stream(file);
	if (!voxelformat::loadFormat(file->name(), stream, newSceneGraph)) {
		Log::error("Failed to load %s", file->name().c_str());
		return false;
	}
	if ((int)newSceneGraph.size() != 1) {
		Log::error("More than one volume/layer found in %s", file->name().c_str());
		return false;
	}
	voxelformat::SceneGraphNode* node = newSceneGraph[0];
	core_assert_always(node != nullptr);
	node->setProperty("type", core::string::toString(volumeIndex));
	sceneGraph.emplace(core::move(*node));
	return true;
}

bool VolumeCache::getVolumes(const animation::AnimationSettings& settings, voxelformat::SceneGraph& sceneGraph) {
	for (size_t i = 0; i < animation::AnimationSettings::MAX_ENTRIES; ++i) {
		if (settings.paths[i].empty()) {
			continue;
		}
		const core::String& fullPath = settings.fullPath(i);
		if (!load(fullPath, (int)i, sceneGraph, settings.meshType(i))) {
			Log::error("Failed to load %s", settings.paths[i].c_str());
			return false;
		}
	}
	return true;
}

}
}
