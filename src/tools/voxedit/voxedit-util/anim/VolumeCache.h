/**
 * @file
 */

#pragma once

#include "voxelformat/VolumeCache.h"
#include "animation/chr/CharacterSettings.h"
#include "animation/chr/CharacterMeshType.h"
#include "voxelformat/VoxelVolumes.h"
#include <memory>

namespace voxedit {
namespace anim {

/**
 * @brief Cache volume instances for @c AnimationEntity
 */
class VolumeCache : public voxelformat::VolumeCache {
private:
	bool load(const std::string& fullPath, size_t volumeIndex, voxel::VoxelVolumes& volumes);
public:
	template<class T>
	bool getCharacterVolumes(const animation::AnimationSettings<T>& settings, voxel::VoxelVolumes& volumes) {
		volumes.resize(std::enum_value(T::Max));

		for (size_t i = 0; i < settings.paths.size(); ++i) {
			if (settings.paths[i].empty()) {
				continue;
			}
			const std::string& fullPath = settings.fullPath((T)i);
			if (!load(fullPath, i, volumes)) {
				Log::error("Failed to load %s", settings.paths[i].c_str());
				return false;
			}
		}
		for (int i = 0; i < std::enum_value(T::Max); ++i) {
			if (volumes[i].volume == nullptr) {
				continue;
			}
			volumes[i].name = toString((T)i);
		}
		return true;
	}
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
}
