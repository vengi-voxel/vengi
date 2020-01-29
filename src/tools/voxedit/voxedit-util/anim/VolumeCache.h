/**
 * @file
 */

#pragma once

#include "voxelformat/VolumeCache.h"
#include "animation/AnimationSettings.h"
#include "voxelformat/VoxelVolumes.h"
#include <memory>

namespace voxedit {
namespace anim {

/**
 * @brief Cache volume instances for @c AnimationEntity
 */
class VolumeCache : public voxelformat::VolumeCache {
private:
	bool load(const core::String& fullPath, size_t volumeIndex, voxel::VoxelVolumes& volumes);
public:
	bool getVolumes(const animation::AnimationSettings& settings, voxel::VoxelVolumes& volumes) {
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
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
}
