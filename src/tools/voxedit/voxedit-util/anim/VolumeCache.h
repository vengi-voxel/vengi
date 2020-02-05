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
	bool getVolumes(const animation::AnimationSettings& settings, voxel::VoxelVolumes& volumes);
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
}
