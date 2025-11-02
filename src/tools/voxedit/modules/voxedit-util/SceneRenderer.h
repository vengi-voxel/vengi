/**
 * @file
 */

#pragma once

#include "core/TimedValue.h"
#include "math/Axis.h"
#include "render/GridRenderer.h"
#include "render/ShapeRenderer.h"
#include "scenegraph/SceneGraph.h"
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
	math::Axis _lockedAxis = math::Axis::None;

	int32_t _planeMeshIndex[3] = {-1, -1, -1};
	int32_t _highlightMeshIndex = -1;
	int32_t _aabbMeshIndex = -1;
	int32_t _boneMeshIndex = -1;
	int32_t _sliceRegionMeshIndex = -1;

	using TimedRegion = core::TimedValue<voxel::Region>;
	TimedRegion _highlightRegion;

	void updateAABBMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx);
	void updateBoneMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx);
	void updateLockedPlane(math::Axis lockedAxis, math::Axis axis, const scenegraph::SceneGraph &sceneGraph,
						   const glm::ivec3 &cursorPosition);
	void updateSliceRegionMesh();
public:
	SceneRenderer();
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
};

} // namespace voxedit
