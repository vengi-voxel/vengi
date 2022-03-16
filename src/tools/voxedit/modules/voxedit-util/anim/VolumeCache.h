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
	bool load(const core::String& fullPath, int volumeIndex, voxelformat::SceneGraph& sceneGraph, const core::String &name);
public:
	bool getVolumes(const animation::AnimationSettings& settings, voxelformat::SceneGraph& sceneGraph);
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
}
