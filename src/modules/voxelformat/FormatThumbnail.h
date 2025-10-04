/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
} // namespace scenegraph

namespace voxelformat {

struct ThumbnailContext {
	glm::ivec2 outputSize{128, 128};
	glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
	float pitch = 0.0f;
	float yaw = 0.0f;
	float roll = 0.0f;
	float distance = -1.0f;
	float farPlane = 5000.0f;
	float sunElevation = 45.0f;
	float sunAzimuth = 135.0f;
	/** This is applied to 2d renderer shots of an image (headless mode) */
	float depthFactor2D = 0.0f;
	/** The camera rotation - see @c deltaFrameSeconds */
	glm::vec3 omega{0.0f, 0.0f, 0.0f};
	core::String cameraMode = "free"; // see voxelrender::SceneCameraModeStr
	/** only used when @c useWorldPosition is set to @c true */
	glm::vec3 worldPosition{0.0f, 0.0f, 0.0f};
	/** This is particularly useful when also using @c omega for the camera rotation */
	double deltaFrameSeconds = 0.001;
	/** Use the scene node camera for rendering the thumbnail */
	bool useSceneCamera = false;
	/** Use @c worldPosition in the camera for rendering the thumbnail */
	bool useWorldPosition = false;
};

/**
 * @brief Callback to create a thumbnail for saving the current scene graph.
 * Some formats supports storing embedded screenshots of the voxel model. This callback
 * must return a RGBA image in the given size
 * @note duplicated in VolumeFormat.h
 */
typedef image::ImagePtr (*ThumbnailCreator)(const scenegraph::SceneGraph &, const ThumbnailContext &);

} // namespace voxelformat
