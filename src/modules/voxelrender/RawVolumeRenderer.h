/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/Region.h"
#include "video/Buffer.h"
#include "VoxelrenderShaders.h"
#include "RenderShaders.h"
#include "voxel/polyvox/Mesh.h"
#include "render/Shadow.h"
#include "video/UniformBuffer.h"
#include "video/Texture.h"
#include "core/GLM.h"

namespace video {
class Camera;
}

namespace voxelrender {

/**
 * @brief Handles the shaders, vertex buffers and rendering of a voxel::RawVolume
 *
 * @sa voxel::RawVolume
 */
class RawVolumeRenderer {
protected:
	static constexpr int MAX_VOLUMES = 4;
	voxel::RawVolume* _rawVolume[MAX_VOLUMES] {};
	glm::mat4 _model[MAX_VOLUMES] {};
	typedef std::array<voxel::Mesh*, MAX_VOLUMES> Meshes;
	typedef std::unordered_map<glm::ivec3, Meshes, std::hash<glm::ivec3> > MeshesMap;
	MeshesMap _meshes;

	video::Buffer _vertexBuffer[MAX_VOLUMES];
	shader::Materialblock _materialBlock;
	shader::WorldShader& _worldShader;
	render::Shadow _shadow;
	core::VarPtr _meshSize;

	video::TexturePtr _whiteTexture;

	int32_t _vertexBufferIndex[MAX_VOLUMES] = {-1, -1, -1, -1};
	int32_t _indexBufferIndex[MAX_VOLUMES] = {-1, -1, -1, -1};

	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);

	void extract(voxel::RawVolume* volume, const voxel::Region& region, voxel::Mesh* mesh) const;

public:
	RawVolumeRenderer();

	void render(const video::Camera& camera, bool shadow = true);

	/**
	 * @brief Updates the vertex buffers manually
	 * @sa extract()
	 */
	bool update(int idx);

	bool extract(int idx, const voxel::Region& region);

	bool toMesh(int idx, voxel::Mesh* mesh);

	/**
	 * @param[in,out] volume The RawVolume pointer
	 * @return The old volume that was managed by the class, @c nullptr if there was none
	 *
	 * @sa volume()
	 */
	voxel::RawVolume* setVolume(int idx, voxel::RawVolume* volume);
	bool setModelMatrix(int idx, const glm::mat4& model);

	bool empty(int idx = 0) const;
	/**
	 * @sa setVolume()
	 */
	voxel::RawVolume* volume(int idx = 0);
	const voxel::RawVolume* volume(int idx = 0) const;

	void setAmbientColor(const glm::vec3& color);

	void construct();

	/**
	 * @sa shutdown()
	 */
	bool init();

	/**
	 * @return the managed voxel::RawVolume instance pointer, or @c nullptr if there is none set.
	 *
	 * @sa init()
	 */
	std::vector<voxel::RawVolume*> shutdown();
};

inline void RawVolumeRenderer::setAmbientColor(const glm::vec3& color) {
	_ambientColor = color;
}

inline voxel::RawVolume* RawVolumeRenderer::volume(int idx) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	return _rawVolume[idx];
}

inline const voxel::RawVolume* RawVolumeRenderer::volume(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	return _rawVolume[idx];
}

}
