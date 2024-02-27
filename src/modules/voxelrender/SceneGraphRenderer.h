/**
 * @file
 */

#pragma once

#include "RawVolumeRenderer.h"
#include "core/collection/DynamicArray.h"
#include "render/CameraFrustum.h"
#include "scenegraph/SceneGraphNode.h"
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
enum class SceneCameraMode : uint8_t { Free, Top, Bottom, Left, Right, Front, Back, Max };
static constexpr const char *SceneCameraModeStr[] = {"Free", "Top", "Bottom", "Left", "Right", "Front", "Back"};
static_assert(lengthof(SceneCameraModeStr) == (int)voxelrender::SceneCameraMode::Max, "Array size doesn't match enum values");

/**
 * @brief Tries to place the camera in a way that most of the scene region is visible in the viewport of the camera.
 */
void configureCamera(video::Camera &camera, const voxel::Region &sceneRegion, SceneCameraMode mode, float farPlane,
					 const glm::vec3 &angles = {0.0f, 0.0f, 0.0f});

/**
 * @brief Rendering of a voxel::SceneGraph
 */
class SceneGraphRenderer : public core::NonCopyable {
protected:
	RawVolumeRenderer _volumeRenderer;
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

	void extractRegion(scenegraph::SceneGraphNode &node, const voxel::Region &region);
	/**
	 * @param waitPending Wait for pending extractions and update the buffers before doing the rendering. If this is
	 * false, you have to call @c update() manually!
	 */
	void render(RenderContext &renderContext, const video::Camera &camera, bool shadow = true,
				bool waitPending = false);
	void clear();
};

} // namespace voxelrender
