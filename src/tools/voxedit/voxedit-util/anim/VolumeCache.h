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
 * @brief Cache volume instances for @c Character
 */
class VolumeCache : public voxelformat::VolumeCache {
private:
	bool load(const std::string& fullPath, animation::CharacterMeshType meshType, voxel::VoxelVolumes& volumes);
public:
	bool getCharacterVolumes(const animation::CharacterSettings& settings, voxel::VoxelVolumes& volumes);
};

using VolumeCachePtr = std::shared_ptr<VolumeCache>;

}
}
