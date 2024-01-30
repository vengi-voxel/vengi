/**
 * @file
 */

#pragma once

#include "RawVolumeRenderer.h"
#include "core/collection/DynamicArray.h"
#include "render/CameraFrustum.h"
#include "scenegraph/SceneGraph.h"
#include "video/Camera.h"

/**
 * Basic voxel rendering
 */
namespace voxelrender {

/**
 * @see voxelformat::toCameraNode()
 */
video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraphNodeCamera &cameraNode);
scenegraph::SceneGraphNodeCamera toCameraNode(const video::Camera &camera);

/**
 * @brief Rendering of a voxel::SceneGraph
 */
class SceneGraphRenderer : public core::NonCopyable {
protected:
	RawVolumeRenderer _renderer;
	render::CameraFrustum _cameraRenderer;
	core::DynamicArray<video::Camera> _cameras;
	void prepare(const RenderContext &renderContext);

public:
	SceneGraphRenderer(const MeshStatePtr &meshState);
	SceneGraphRenderer();
	void construct();
	bool init();
	void update();
	void shutdown();

	void setAmbientColor(const glm::vec3 &color);
	void setDiffuseColor(const glm::vec3 &color);

	void nodeRemove(int nodeId);

	bool extractRegion(scenegraph::SceneGraphNode &node, const voxel::Region &region);
	void extractAll();
	/**
	 * @param waitPending Wait for pending extractions and update the buffers before doing the rendering. If this is
	 * false, you have to call @c update() manually!
	 */
	void render(RenderContext &renderContext, const video::Camera &camera, bool shadow = true,
				bool waitPending = false);
	void clear();
};

} // namespace voxelrender
