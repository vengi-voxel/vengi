/**
 * @file
 */

#pragma once

#include "voxelformat/MeshCache.h"
#include "AnimationSettings.h"
#include "core/Assert.h"
#include "Vertex.h"
#include <string>
#include <memory>

namespace animation {

/**
 * @brief Cache @c voxel::Mesh instances for @c AnimationEntity
 */
class AnimationCache : public voxelformat::MeshCache {
protected:
	bool load(const std::string& filename, size_t meshIndex, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES]);

	bool getMeshes(const AnimationSettings& settings, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES],
			std::function<bool(const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES])> loadAdditional = {});

public:
	bool getModel(const AnimationSettings& settings, const char *fullPath, BoneId boneId, Vertices& vertices, Indices& indices);

	bool getBoneModel(const AnimationSettings& settings, Vertices& vertices, Indices& indices,
			std::function<bool(const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES])> loadAdditional = {});
};

using AnimationCachePtr = std::shared_ptr<AnimationCache>;

}
