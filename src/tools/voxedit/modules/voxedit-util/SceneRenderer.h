/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/TimedValue.h"
#include "math/Axis.h"
#include "render/GridRenderer.h"
#include "render/ShapeRenderer.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "video/ShapeBuilder.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/SceneGraphRenderer.h"

namespace voxedit {

/**
 * @brief The scene renderer is responsible for rendering the scene graph
 * and the voxel volumes. It is used by the scene manager.
 *
 * @sa ISceneRenderer
 */
class SceneRenderer : public ISceneRenderer {
private:
	voxel::MeshStatePtr _meshState;
	voxelrender::SceneGraphRenderer _sceneGraphRenderer;
	render::GridRenderer _gridRenderer;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;

	core::VarPtr _showGrid;
	core::VarPtr _showLockedAxis;
	core::VarPtr _showAABB;
	core::VarPtr _showBones;
	core::VarPtr _renderShadow;
	core::VarPtr _shadingMode;
	core::VarPtr _gridSize;
	core::VarPtr _grayInactive;
	core::VarPtr _hideInactive;
	core::VarPtr _ambientColor;
	core::VarPtr _sunAngle;
	core::VarPtr _diffuseColor;
	core::VarPtr _planeSize;
	core::VarPtr _showPlane;

	/** @brief The currently locked axis for modifications. */
	math::Axis _lockedAxis = math::Axis::None;
	/** @brief The tracked 3D cursor position in the voxel scene. */
	glm::ivec3 _cursorPosition{0};

	/** @brief Shape renderer mesh index */
	struct ShapeIndices {
		/** @brief for the locked mathematical planes (X, Y, Z). */
		int32_t plane[3] = {-1, -1, -1};
		/** @brief for the highlighted active region. */
		int32_t highlight = -1;
		/** @brief for the global scene or node bounding box. */
		int32_t aabb = -1;
		/** @brief for the skeletal bone connections. */
		int32_t bone = -1;
		/** @brief for visualizing the active slice region. */
		int32_t sliceRegion = -1;
	} _indices;

	/** @brief The AABB representing the current grid rendering bounds. */
	math::AABB<float> _nextGridRegionUpdate;

	using TimedRegion = core::TimedValue<voxel::Region>;
	/** @brief A region highlight that fades out over time (used for visual feedback on modifications). */
	TimedRegion _highlightRegion;

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
	} _cache;

	void updateAABBMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx);
	void updateBoneMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx);
	void updateLockedPlane(math::Axis lockedAxis, math::Axis axis, const scenegraph::SceneGraph &sceneGraph,
						   const glm::ivec3 &cursorPosition);
	void updateSliceRegionMesh();

public:
	SceneRenderer(const core::TimeProviderPtr &timeProvider);
	virtual ~SceneRenderer() = default;

	// IComponent
	void construct() override;
	bool init() override;
	void shutdown() override;

	// ISceneRenderer
	void update() override;
	void clear() override;
	void updateLockedPlanes(math::Axis lockedAxis, const scenegraph::SceneGraph &sceneGraph,
							const glm::ivec3 &cursorPosition) override;
	void updateNodeRegion(int nodeId, const voxel::Region &region, uint64_t renderRegionMillis = 0) override;
	void updateGridRegion(const voxel::Region &region) override;
	void removeNode(int nodeId) override;
	bool isVisible(int nodeId, bool hideEmpty = true) const override;
	void renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera) override;
	void renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera) override;
	const voxel::RawVolume *volumeForNode(const scenegraph::SceneGraphNode &node) override;
	const voxel::Region &sliceRegion() const override;
	void setSliceRegion(const voxel::Region &region) override;
	bool isSliceModeActive() const override;
	RendererStats rendererStats() const override;
	void markDirty() override;
	void unhideNode(int nodeId) override;
};

} // namespace voxedit
