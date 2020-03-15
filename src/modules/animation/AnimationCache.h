/**
 * @file
 */

#pragma once

#include "voxelformat/MeshCache.h"
#include "AnimationSettings.h"
#include "core/Assert.h"
#include "Vertex.h"
#include "core/String.h"
#include <memory>

namespace animation {

/**
 * @brief Cache @c voxel::Mesh instances for @c AnimationEntity
 * @ingroup Animation
 */
class AnimationCache : public core::IComponent {
protected:
	/**
	 * @brief Load from cache or file and extract the mesh
	 */
	bool load(const core::String& filename, size_t meshIndex, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES]);

	/**
	 * @brief Load and cache the voxel meshes that are needed to assmble the model as
	 * defined by the given AnimationSettings
	 */
	bool getMeshes(const AnimationSettings& settings, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES],
			const std::function<bool(const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES])>& loadAdditional = {});

	voxelformat::MeshCachePtr _meshCache;

public:
	AnimationCache(const voxelformat::MeshCachePtr& meshCache);

	const voxel::Mesh* getMesh(const char *fullPath);
	bool removeMesh(const char *fullPath);
	bool putMesh(const char* fullPath, const voxel::Mesh& mesh);
	bool init() override;
	void shutdown() override;

	/**
	 * @brief Map a single bone to the given vertices and fill the vertex indices
	 */
	bool getModel(const AnimationSettings& settings, const char *fullPath, BoneId boneId, Vertices& vertices, Indices& indices);

	/**
	 * @brief Map the bone indices to the vertices of the mesh and fill the vertex indices
	 */
	bool getBoneModel(const AnimationSettings& settings, Vertices& vertices, Indices& indices,
			const std::function<bool(const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES])>& loadAdditional = {});
};

using AnimationCachePtr = std::shared_ptr<AnimationCache>;

}
