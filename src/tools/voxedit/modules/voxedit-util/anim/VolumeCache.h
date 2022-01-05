/**
 * @file
 */

#pragma once

#include "voxelformat/VolumeCache.h"
#include "animation/AnimationSettings.h"
#include "voxelformat/SceneGraph.h"
#include <memory>

namespace voxedit {
namespace anim {

/**
 * @brief Cache volume instances for @c AnimationEntity
 */
class VolumeCache : public voxelformat::VolumeCache {
private:
	bool load(const core::String& fullPath, size_t volumeIndex, voxel::SceneGraph& volumes);
public:
	bool getVolumes(const animation::AnimationSettings& settings, voxel::SceneGraph& volumes);
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
}
