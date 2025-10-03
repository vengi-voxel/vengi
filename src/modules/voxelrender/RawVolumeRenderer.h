/**
 * @file
 */

#pragma once

#include "render/ShapeRenderer.h"
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

enum class RenderMode : uint8_t {
	Edit, Scene, Max
};

struct RenderContext : public core::NonCopyable {
	video::FrameBuffer frameBuffer;        // Main framebuffer (multisampled when MSAA is enabled)
	video::FrameBuffer resolveFrameBuffer; // Resolve target for multisampled framebuffer
	render::BloomRenderer bloomRenderer;
	const scenegraph::SceneGraph *sceneGraph = nullptr;
	scenegraph::FrameIndex frame = 0;
	bool hideInactive = false;
	bool grayInactive = false;
	bool onlyModels = false;
	// render the built-in normals
	bool renderNormals = false;
	bool applyTransformsInEditMode = true;
	RenderMode renderMode = RenderMode::Edit;
	// multisampling configuration
	bool enableMultisampling = false;
	int multisampleSamples = 4;

	bool isEditMode() const;
	bool isSceneMode() const;
	bool showCameras() const;
	bool applyTransforms() const;

	bool init(const glm::ivec2 &size);
	void shutdown();
	bool resize(const glm::ivec2 &size);
	bool updateMultisampling();
};

/**
 * @brief Handles the shaders, vertex buffers and rendering of a voxel::RawVolume - including the mesh extraction part
 * @sa MeshRenderer
 * @sa voxel::RawVolume
 */
class RawVolumeRenderer : public core::NonCopyable {
protected:
	struct RenderState : public core::NonCopyable {
		bool _culled = false;
		bool _empty = false; // this is only updated for non hidden nodes
		bool _dirtyNormals = false;
		int32_t _vertexBufferIndex[voxel::MeshType_Max]{-1, -1};
		int32_t _normalBufferIndex[voxel::MeshType_Max]{-1, -1};
		int32_t _normalPreviewBufferIndex = -1;
		int32_t _indexBufferIndex[voxel::MeshType_Max]{-1, -1};
		video::Buffer _vertexBuffer[voxel::MeshType_Max];

		uint32_t indices(voxel::MeshType type) const {
			return _vertexBuffer[type].elements(_indexBufferIndex[type], 1, sizeof(voxel::IndexType));
		}

		bool hasData() const {
			return indices(voxel::MeshType_Opaque) > 0 || indices(voxel::MeshType_Transparency) > 0;
		}
	};
	core::Array<RenderState, voxel::MAX_VOLUMES> _state{};

	uint64_t _paletteHash = 0;
	uint32_t _normalsPaletteHash = 0;

	shader::VoxelData _voxelData;

	alignas(16) shader::VoxelData::FragData _voxelShaderFragData;
	alignas(16) shader::VoxelData::VertData _voxelShaderVertData;

	shader::VoxelShader &_voxelShader;
	shader::VoxelnormShader &_voxelNormShader;
	shader::ShadowmapData _shadowMapUniformBlock;
	shader::ShadowmapShader &_shadowMapShader;
	voxelrender::Shadow _shadow;

	render::ShapeRenderer _shapeRenderer;
	video::ShapeBuilder _shapeBuilder;

	core::VarPtr _shadowMap;
	core::VarPtr _bloom;

	void updatePalette(const voxel::MeshStatePtr &meshState, int idx);
	enum UpdateBufferFlags : uint8_t {
		Vertices = 1 << 0,
		Normals = 1 << 1,
		Indices = 1 << 2,
		All = Vertices | Normals | Indices
	};
	bool updateBufferForVolume(const voxel::MeshStatePtr &meshState, int idx, voxel::MeshType type, UpdateBufferFlags flags = UpdateBufferFlags::All);
	void deleteMesh(int idx, voxel::MeshType meshType);
	void deleteMeshes(int idx);
	void updateCulling(const voxel::MeshStatePtr &meshState, int idx, const video::Camera &camera);

	bool initStateBuffers(bool normals);
	void shutdownStateBuffers();
	bool resetStateBuffers(bool normals);
	/**
	 * @brief Updates the vertex buffers manually
	 */
	bool updateBufferForVolume(const voxel::MeshStatePtr &meshState, int idx);
	void renderOpaque(const voxel::MeshStatePtr &meshState, const video::Camera &camera);
	void renderTransparency(const voxel::MeshStatePtr &meshState, RenderContext &renderContext, const video::Camera &camera);
	void renderNormals(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext, const video::Camera &camera);
public:
	RawVolumeRenderer();

	void sortBeforeRender(const voxel::MeshStatePtr &meshState, const video::Camera &camera);
	void render(const voxel::MeshStatePtr &meshState, RenderContext &renderContext, const video::Camera &camera,
				bool shadow);
	void clear(const voxel::MeshStatePtr &meshState);
	/**
	 * @brief Checks whether the given volume @c idx is visible
	 * @param[in] idx The volume index
	 * @param[in] hideEmpty If @c true, the function will return @c false if the volume is empty
	 * @return @c true if the volume is visible, @c false otherwise
	 */
	bool isVisible(const voxel::MeshStatePtr &meshState, int idx, bool hideEmpty = true) const;

	void scheduleRegionExtraction(const voxel::MeshStatePtr &meshState, int idx, const voxel::Region& region);

	/**
	 * @param[in,out] volume The RawVolume pointer
	 * @return The old volume that was managed by the class, @c nullptr if there was none
	 */
	[[nodiscard]] voxel::RawVolume *setVolume(const voxel::MeshStatePtr &meshState, int idx, voxel::RawVolume *volume,
											  palette::Palette *palette, palette::NormalPalette *normalPalette,
											  bool meshDelete);
	void setVolume(const voxel::MeshStatePtr &meshState, int idx, scenegraph::SceneGraphNode &node, bool deleteMesh);

	/**
	 * @return The old volume that was managed by the class, @c nullptr if there was none
	 */
	[[nodiscard]] voxel::RawVolume *resetVolume(const voxel::MeshStatePtr &meshState, int idx);

	void setAmbientColor(const glm::vec3 &color);
	void setDiffuseColor(const glm::vec3 &color);
	void setSunAngle(const glm::vec3 &angle);

	void construct();

	/**
	 * @sa shutdown()
	 */
	bool init(bool normals);

	void update(const voxel::MeshStatePtr &meshState);

	/**
	 * @sa init()
	 */
	void shutdown();
};

} // namespace voxelrender
