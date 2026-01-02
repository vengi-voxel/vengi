/**
 * @file
 */

#pragma once

#include "app/I18N.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "video/Camera.h"
#include "voxel/Region.h"

namespace voxelrender {

/**
 * @see voxelformat::toCameraNode()
 */
video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraphNodeCamera &cameraNode);
video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraph &sceneGraph,
					   const scenegraph::SceneGraphNodeCamera &cameraNode, scenegraph::FrameIndex frameIdx);
scenegraph::SceneGraphNodeCamera toCameraNode(const video::Camera &camera);
enum class SceneCameraMode : uint8_t { Free, Top, Bottom, Left, Right, Front, Back, Max };
// I18N: These are not translated, because they are also the values of configuration variables
static constexpr const char *SceneCameraModeStr[] = {N_("Free"),  N_("Top"),   N_("Bottom"), N_("Left"),
													 N_("Right"), N_("Front"), N_("Back")};
static_assert(lengthof(SceneCameraModeStr) == (int)voxelrender::SceneCameraMode::Max,
			  "Array size doesn't match enum values");

/**
 * @brief Tries to place the camera in a way that most of the scene region is visible in the viewport of the camera.
 * @param[in,out] camera The camera to configure
 * @param[in] angles The angles to set the camera to (pitch, yaw, roll in radians)
 */
void configureCamera(video::Camera &camera, const voxel::Region &sceneRegion, SceneCameraMode mode, float farPlane,
					 const glm::vec3 &angles = {0.0f, 0.0f, 0.0f});

} // namespace voxelrender
