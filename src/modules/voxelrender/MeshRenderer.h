/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "VoxelrenderShaders.h"
#include "RenderShaders.h"
#include "render/Shadow.h"
#include "core/GLM.h"
#include "voxel/VoxelVertex.h"
#include "core/collection/Array.h"

namespace video {
class Camera;
}

namespace voxel {
class Mesh;
}

namespace voxelrender {

/**
 * @brief Handles the shaders, vertex buffers and rendering of a voxel::Mesh
 *
 * @sa voxel::Mesh
 */
class MeshRenderer : public core::IComponent {
protected:
	struct MeshInternal {
		const voxel::Mesh* mesh;
		video::Buffer buffer;
		glm::mat4 model;
		int32_t vbo = -1;
		int32_t ibo = -1;

		inline uint32_t numIndices() const {
			return buffer.elements(ibo, 1, sizeof(voxel::IndexType));
		}
	};
	core::Array<MeshInternal, 64> _meshes;
	int _meshIndex = 0;
	shader::VoxelData _materialBlock;
	shader::VoxelShader& _voxelShader;
	shader::ShadowmapShader& _shadowMapShader;
	render::Shadow _shadow;

	bool isEmpty() const;
	bool update(int idx, const voxel::VoxelVertex* vertices, size_t numVertices, const voxel::IndexType* indices, size_t numIndices);
	void prepareState();
	void renderShadows(const video::Camera& camera);
	void prepareShader(const video::Camera& camera);
public:
	MeshRenderer();
	void renderAll(const video::Camera& camera);
	void render(int idx, const video::Camera& camera);
	/**
	 * @brief The renderer has a fixed size buffer for meshes - if you add more than the
	 * allowed amount of meshes, the first one will get overridden
	 */
	int addMesh(const voxel::Mesh* mesh, const glm::mat4& model = glm::mat4(1.0f));
	/**
	 * @note The caller owns the mesh pointer
	 */
	bool setMesh(int idx, const voxel::Mesh* mesh, const glm::mat4& model = glm::mat4(1.0f));
	bool setModelMatrix(int idx, const glm::mat4& model = glm::mat4(1.0f));
	bool init() override;
	void shutdown() override;

	int maxMeshes() const;
	int meshIndex() const;
};

inline int MeshRenderer::meshIndex() const {
	return _meshIndex;
}

inline int MeshRenderer::maxMeshes() const {
	return _meshes.size();
}

}
