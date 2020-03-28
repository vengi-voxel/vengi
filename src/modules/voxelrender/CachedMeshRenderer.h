/**
 * @file
 */

#pragma once

#include "voxelformat/MeshCache.h"
#include "voxelrender/MeshRenderer.h"
#include "core/IComponent.h"
#include <glm/mat4x4.hpp>
#include "core/SharedPtr.h"

namespace voxelrender {

class CachedMeshRenderer : public core::IComponent {
private:
	voxelformat::MeshCachePtr _meshCache;
	voxelrender::MeshRenderer _meshRenderer;
public:
	CachedMeshRenderer(const voxelformat::MeshCachePtr& meshCache);

	bool init();
	void shutdown();

	bool removeMesh(int index);
	/**
	 * @param[in] fullpath The path of the voxel mes
	 * @return The returned index can be used to render the particular mesh, remove it, or update it
	 */
	int addMesh(const char *fullpath, const glm::mat4& model = glm::mat4(1.0f));
	/**
	 * @param[in] idx The index of the model returned by @c addMesh()
	 * @param[in] model The new model matrix
	 * @return If the update was successful, return @c true otherwise @c false
	 */
	bool setModelMatrix(int idx, const glm::mat4& model);

	void renderAll(const video::Camera& camera);
	void render(int idx, const video::Camera& camera);
};

using CachedMeshRendererPtr = core::SharedPtr<CachedMeshRenderer>;

}
