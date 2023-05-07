/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
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

class SceneRenderer : public ISceneRenderer {
private:
	voxelrender::SceneGraphRenderer _volumeRenderer;
	render::GridRenderer _gridRenderer;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;

	core::VarPtr _showGrid;
	core::VarPtr _showLockedAxis;
	core::VarPtr _showAABB;
	core::VarPtr _renderShadow;
	core::VarPtr _gridSize;
	core::VarPtr _grayInactive;
	core::VarPtr _hideInactive;
	core::VarPtr _ambientColor;
	core::VarPtr _diffuseColor;

	int32_t _planeMeshIndex[3] = {-1, -1, -1};
	int32_t _highlightMeshIndex = -1;
	int32_t _aabbMeshIndex = -1;

	struct DirtyRegion {
		voxel::Region region;
		int nodeId;
	};
	using RegionQueue = core::DynamicArray<DirtyRegion>;
	RegionQueue _extractRegions;

	using TimedRegion = core::TimedValue<voxel::Region>;
	TimedRegion _highlightRegion;

	void updateAABBMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx);
	bool extractVolume(const scenegraph::SceneGraph &sceneGraph);
	void updateLockedPlane(math::Axis lockedAxis, math::Axis axis, const scenegraph::SceneGraph &sceneGraph,
						   const glm::ivec3 &cursorPosition);

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
	void nodeRemove(int nodeId) override;
	void renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera,
				  const scenegraph::SceneGraph &sceneGraph) override;
	void renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera,
					 const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frame) override;
};

} // namespace voxedit
