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
		voxel::Mesh* mesh;
		video::Buffer buffer;
		glm::mat4 model;
		int32_t vbo = -1;
		int32_t ibo = -1;

		inline uint32_t numIndices() const {
			return buffer.elements(ibo, 1, sizeof(voxel::IndexType));
		}
	};
	core::Array<MeshInternal, 64> _meshes;
	shader::VoxelData _materialBlock;
	shader::VoxelShader& _voxelShader;
	shader::ShadowmapShader& _shadowMapShader;
	render::Shadow _shadow;

	bool isEmpty() const;
	bool update(int idx, const voxel::VoxelVertex* vertices, size_t numVertices, const voxel::IndexType* indices, size_t numIndices);

public:
	MeshRenderer();
	void renderAll(const video::Camera& camera);
	/**
	 * @note The caller owns the mesh pointer
	 */
	bool setMesh(int idx, voxel::Mesh* mesh, const glm::mat4& model = glm::mat4(1.0f));
	bool init() override;
	void shutdown() override;
};

}
