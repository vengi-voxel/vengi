/**
 * @file
 */

#pragma once

#include "voxel/MeshState.h"
#include "ShadowmapData.h"
#include "ShadowmapShader.h"
#include "VoxelShader.h"
#include "VoxelnormShader.h"
#include "core/NonCopyable.h"
#include "core/Var.h"
#include "core/collection/Array.h"
#include "render/BloomRenderer.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "video/Buffer.h"
#include "video/FrameBuffer.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelrender/Shadow.h"

namespace video {
class Camera;
}

namespace scenegraph {
class SceneGraphNode;
class SceneGraph;
}

namespace palette {
class Palette;
}

/**
 * Basic voxel rendering
 */
namespace voxelrender {

struct RenderContext : public core::NonCopyable {
	video::FrameBuffer frameBuffer;
	render::BloomRenderer bloomRenderer;
	const scenegraph::SceneGraph *sceneGraph = nullptr;
	scenegraph::FrameIndex frame = 0;
	bool hideInactive = false;
	bool grayInactive = false;
	bool sceneMode = false;
	bool onlyModels = false;

	bool init(const glm::ivec2 &size);
	void shutdown();
	bool resize(const glm::ivec2 &size);
};

/**
 * @brief Handles the shaders, vertex buffers and rendering of a voxel::RawVolume - including the mesh extraction part
 * @sa MeshRenderer
 * @sa voxel::RawVolume
 */
class RawVolumeRenderer : public core::NonCopyable {
protected:
	struct State {
		bool _culled = false;
		int32_t _vertexBufferIndex[voxel::MeshType_Max]{-1, -1};
		int32_t _normalBufferIndex[voxel::MeshType_Max]{-1, -1};
		int32_t _indexBufferIndex[voxel::MeshType_Max]{-1, -1};
		video::Buffer _vertexBuffer[voxel::MeshType_Max];

		uint32_t indices(voxel::MeshType type) const {
			return _vertexBuffer[type].elements(_indexBufferIndex[type], 1, sizeof(voxel::IndexType));
		}

		bool hasData() const {
			return indices(voxel::MeshType_Opaque) > 0 || indices(voxel::MeshType_Transparency) > 0;
		}
	};
	core::Array<State, voxel::MAX_VOLUMES> _state{};
	core::SharedPtr<voxel::MeshState> _meshState;

	uint64_t _paletteHash = 0;

	shader::VoxelData _voxelData;

	alignas(16) shader::VoxelData::FragData _voxelShaderFragData;
	alignas(16) shader::VoxelData::VertData _voxelShaderVertData;

	shader::VoxelShader &_voxelShader;
	shader::VoxelnormShader &_voxelNormShader;
	shader::ShadowmapData _shadowMapUniformBlock;
	shader::ShadowmapShader &_shadowMapShader;
	voxelrender::Shadow _shadow;

	core::VarPtr _shadowMap;
	core::VarPtr _bloom;

	void updatePalette(int idx);
	bool updateBufferForVolume(int idx, voxel::MeshType type);
	void deleteMesh(int idx, voxel::MeshType meshType);
	void deleteMeshes(int idx);
	void updateCulling(int idx, const video::Camera &camera);

	bool initStateBuffers();
	void shutdownStateBuffers();
	bool resetStateBuffers();
	/**
	 * @brief Updates the vertex buffers manually
	 * @sa extract()
	 */
	bool updateBufferForVolume(int idx);
public:
	RawVolumeRenderer();
	RawVolumeRenderer(const voxel::MeshStatePtr &meshState);

	void render(RenderContext &renderContext, const video::Camera &camera, bool shadow);
	void clear();
	bool isVisible(int idx) const;

	void scheduleRegionExtraction(int idx, const voxel::Region& region);

	/**
	 * @param[in,out] volume The RawVolume pointer
	 * @return The old volume that was managed by the class, @c nullptr if there was none
	 *
	 * @sa volume()
	 */
	voxel::RawVolume *setVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette, bool meshDelete);
	void setVolume(int idx, const scenegraph::SceneGraphNode& node, bool deleteMesh);

	void resetVolume(int idx);

	void setAmbientColor(const glm::vec3 &color);
	void setDiffuseColor(const glm::vec3 &color);
	void setSunPosition(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up);

	void construct();

	/**
	 * @sa shutdown()
	 */
	bool init();

	void update();

	/**
	 * @return the managed voxel::RawVolume instance pointer, or @c nullptr if there is none set.
	 * @note You take the ownership of the returned volume pointers. Don't forget to delete them.
	 *
	 * @sa init()
	 */
	core::DynamicArray<voxel::RawVolume *> shutdown();

	voxel::MeshStatePtr meshState() const;
};

inline voxel::MeshStatePtr RawVolumeRenderer::meshState() const {
	return _meshState;
}

} // namespace voxelrender
