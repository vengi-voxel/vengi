/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "video/Buffer.h"
#include "VoxelrenderShaders.h"
#include "voxel/Mesh.h"
#include "render/Shadow.h"
#include "video/UniformBuffer.h"
#include "video/Texture.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "core/collection/Array.h"

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
public:
	static constexpr int MAX_VOLUMES = 64;
protected:
	voxel::RawVolume* _rawVolume[MAX_VOLUMES] {};
	glm::mat4 _model[MAX_VOLUMES] {};
	core::Array<bool, MAX_VOLUMES> _hidden {{ false }};
	typedef core::Array<voxel::Mesh*, MAX_VOLUMES> Meshes;
	typedef std::unordered_map<glm::ivec3, Meshes> MeshesMap;
	MeshesMap _meshes;

	video::Buffer _vertexBuffer[MAX_VOLUMES];
	shader::VoxelData _materialBlock;
	shader::VoxelShader& _voxelShader;
	render::Shadow _shadow;
	core::VarPtr _meshSize;
	core::VarPtr _shadowVar;

	int32_t _vertexBufferIndex[MAX_VOLUMES] = {-1};
	int32_t _indexBufferIndex[MAX_VOLUMES] = {-1};

	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);

	void extract(voxel::RawVolume* volume, const voxel::Region& region, voxel::Mesh* mesh) const;

public:
	RawVolumeRenderer();

	void render(const video::Camera& camera, bool shadow = true);
	void hide(int idx, bool hide);
	bool hiddenState(int idx) const;

	render::Shadow& shadow();
	const render::Shadow& shadow() const;

	/**
	 * @brief Updates the vertex buffers manually
	 * @sa extract()
	 */
	bool update(int idx);

	bool update(int idx, const std::vector<voxel::VoxelVertex>& vertices, const std::vector<voxel::IndexType>& indices);

	bool extract(int idx, const voxel::Region& region, bool updateBuffers = true);

	bool translate(int idx, const glm::ivec3& m);

	bool toMesh(voxel::Mesh* mesh);
	bool toMesh(int idx, voxel::Mesh* mesh);

	/**
	 * @param[in,out] volume The RawVolume pointer
	 * @return The old volume that was managed by the class, @c nullptr if there was none
	 *
	 * @sa volume()
	 */
	voxel::RawVolume* setVolume(int idx, voxel::RawVolume* volume, bool deleteMesh = true);
	bool setModelMatrix(int idx, const glm::mat4& model);

	bool empty(int idx = 0) const;
	bool swap(int idx1, int idx2);
	/**
	 * @sa setVolume()
	 */
	voxel::RawVolume* volume(int idx = 0);
	const voxel::RawVolume* volume(int idx = 0) const;
	/**
	 * @note If you need the region of a particular volume, ask the volume
	 * @return The complete region of all managed volumes
	 */
	voxel::Region region() const;

	void setAmbientColor(const glm::vec3& color);
	void setDiffuseColor(const glm::vec3& color);
	void setSunPosition(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

	void construct();

	/**
	 * @sa shutdown()
	 */
	bool init();

	/**
	 * @return the managed voxel::RawVolume instance pointer, or @c nullptr if there is none set.
	 * @note You take the ownership of the returned volume pointers. Don't forget to delete them.
	 *
	 * @sa init()
	 */
	std::vector<voxel::RawVolume*> shutdown();
};

inline render::Shadow& RawVolumeRenderer::shadow() {
	return _shadow;
}

inline const render::Shadow& RawVolumeRenderer::shadow() const {
	return _shadow;
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
