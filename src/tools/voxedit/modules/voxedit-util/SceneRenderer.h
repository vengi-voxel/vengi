/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "math/Axis.h"
#include "render/ShapeRenderer.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "video/ShapeBuilder.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/AddNodePreview.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/SceneGraphRenderer.h"
#include <cstdint>

namespace voxedit {

/**
 * @brief The scene renderer is responsible for rendering the scene graph
 * and the voxel volumes. It is used by the scene manager.
 *
 * @sa ISceneRenderer
 */
class SceneRenderer : public ISceneRenderer {
private:
	using Super = ISceneRenderer;

	voxel::MeshStatePtr _meshState;
	voxelrender::SceneGraphRenderer _sceneGraphRenderer;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;

	core::VarPtr _showAABB;
	core::VarPtr _showBones;
	core::VarPtr _renderShadow;
	core::VarPtr _shadingMode;
	core::VarPtr _grayInactive;
	core::VarPtr _hideInactive;
	core::VarPtr _ambientColor;
	core::VarPtr _sunAngle;
	core::VarPtr _diffuseColor;

	/** @brief Shape renderer mesh index */
	struct ShapeIndices {
		/** @brief for the global scene or node bounding box. */
		int32_t aabb = -1;
		/** @brief for the skeletal bone connections. */
		int32_t bone = -1;
		/** @brief for visualizing the active slice region. */
		int32_t sliceRegion = -1;
		/** @brief for add-node mode face highlight and preview. */
		int32_t addNodeFace = -1;
		int32_t addNodePreview = -1;
	} _indices;

	AddNodePreview _addNodePreview;

	struct Cache {
		/** @brief Flag indicating if the AABB visualization mesh needs to be rebuilt. */
		bool aabbDirty = true;
		/** @brief Flag indicating if the skeletal bone visualization mesh needs to be rebuilt. */
		bool boneDirty = true;
		/** @brief Caches the frame index for which the AABB mesh was last generated. */
		scenegraph::FrameIndex lastAABBFrame = InvalidFrame;
		/** @brief Caches the frame index for which the bone mesh was last generated. */
		scenegraph::FrameIndex lastBoneFrame = InvalidFrame;
		/** @brief Caches the active node ID used during the last AABB mesh generation to detect swaps. */
		int lastAABBActiveNode = InvalidNodeId;
		/** @brief Caches the hide inactive state to trigger AABB rebuilds when toggled. */
		bool lastHideInactive = false;
		/** @brief Caches the gray inactive state to trigger AABB/Bone rebuilds when toggled. */
		bool lastGrayInactive = false;
		AddNodePreview lastAddNodePreview;
	} _cache;

	void doUpdateAABBMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx);
	void doUpdateBoneMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx);
	void doUpdateSliceRegionMesh();
	void doUpdateAddNodePreviewMesh();

	void handleCommandBuffer();
	void checkMainThread() const;

public:
	SceneRenderer(const core::TimeProviderPtr &timeProvider);
	virtual ~SceneRenderer() = default;

	// IComponent
	void construct() override;
	bool init() override;
	void shutdown() override;

	// ISceneRenderer
	void update() override;
	bool isVisible(int nodeId, bool hideEmpty = true) const override;
	void renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera) override;
	void renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera) override;
	const voxel::RawVolume *volumeForNode(const scenegraph::SceneGraphNode &node) override;
	const voxel::Region &sliceRegion() const override;
	RendererStats rendererStats() const override;
	void setAddNodePreview(const AddNodePreview &preview) override;
};

} // namespace voxedit
