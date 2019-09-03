/**
 * @file
 */

#pragma once

#include "voxel/polyvox/PagedVolume.h"
#include "video/Buffer.h"
#include "VoxelrenderShaders.h"
#include "RenderShaders.h"
#include "voxel/polyvox/Mesh.h"
#include "render/Shadow.h"
#include "video/UniformBuffer.h"
#include "video/Texture.h"
#include "core/IComponent.h"
#include "core/GLM.h"

namespace video {
class Camera;
}

namespace voxelrender {

/**
 * @brief Handles the shaders, vertex buffers and rendering of a voxel::PagedVolume
 *
 * @sa voxel::PagedVolume
 */
class PagedVolumeRenderer : public core::IComponent {
protected:
	voxel::PagedVolume* _volume = nullptr;
	voxel::Mesh* _mesh = nullptr;

	video::Buffer _vertexBuffer;
	shader::WorldData _materialBlock;
	shader::WorldShader& _worldShader;
	render::Shadow _shadow;

	video::TexturePtr _whiteTexture;

	int32_t _vertexBufferIndex = -1;
	int32_t _indexBufferIndex = -1;

	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);
public:
	PagedVolumeRenderer();

	void render(const video::Camera& camera);

	/**
	 * @brief Updates the vertex buffers manually
	 * @sa extract()
	 */
	bool update(const std::vector<voxel::VoxelVertex>& vertices, const std::vector<voxel::IndexType>& indices);
	bool update();

	/**
	 * @brief Reextract the whole volume region and updates the vertex buffers.
	 * @sa update()
	 */
	bool extract();

	const voxel::Mesh* mesh() const;
	voxel::PagedVolume* setVolume(voxel::PagedVolume* volume);
	voxel::PagedVolume* volume();
	const voxel::PagedVolume* volume() const;

	void setAmbientColor(const glm::vec3& color);

	bool init() override;
	void shutdown() override;

	bool onResize(const glm::ivec2& position, const glm::ivec2& dimension);
};

inline void PagedVolumeRenderer::setAmbientColor(const glm::vec3& color) {
	_ambientColor = color;
}

inline voxel::PagedVolume* PagedVolumeRenderer::volume() {
	return _volume;
}

inline const voxel::PagedVolume* PagedVolumeRenderer::volume() const {
	return _volume;
}

inline const voxel::Mesh* PagedVolumeRenderer::mesh() const {
	return _mesh;
}

}
