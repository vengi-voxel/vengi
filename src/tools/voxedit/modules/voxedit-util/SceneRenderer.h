/**
 * @file
 */

#include "core/TimedValue.h"
#include "math/Axis.h"
#include "render/GridRenderer.h"
#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "scenegraph/SceneGraph.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/SceneGraphRenderer.h"

namespace voxedit {

class SceneRenderer {
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
	void updateLockedPlane(math::Axis lockedAxis, math::Axis axis, const scenegraph::SceneGraph &sceneGraph, const glm::ivec3& cursorPosition);
public:
	SceneRenderer();

	void construct();
	bool init();
	void update();
	void clear();
	void shutdown();

	void updateLockedPlanes(math::Axis lockedAxis, const scenegraph::SceneGraph &sceneGraph, const glm::ivec3& cursorPosition);
	void updateNodeRegion(int nodeId, const voxel::Region &region, uint64_t renderRegionMillis = 0);
	void updateGridRegion(const voxel::Region &region);

	void renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera,
				  const scenegraph::SceneGraph &sceneGraph);
	void renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera,
					 const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frame);
};

} // namespace voxedit
