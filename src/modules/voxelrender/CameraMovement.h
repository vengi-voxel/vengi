/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/Var.h"
#include "scenegraph/Physics.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/Movement.h"
#include "video/Camera.h"

namespace voxelrender {

/**
 * @brief Handles camera movement logic including physics-based movement.
 *
 * This component manages the camera's position and orientation updates based on user input
 * and physics simulation. It supports features like:
 *
 * @li Walking and running with configurable speed.
 * @li Jumping and gravity application.
 * @li Collision detection and response (clipping).
 * @li "Eye mode" or "Game mode" style control.
 *
 * @sa Viewport::isGameMode()
 */
class CameraMovement : public core::IComponent {
protected:
	core::VarPtr _movementSpeed;
	core::VarPtr _jumpVelocity;
	core::VarPtr _bodyHeight;
	core::VarPtr _gravity;
	core::VarPtr _friction;
	core::VarPtr _bodySize;
	core::VarPtr _applyGravity;
	core::VarPtr _clipping;
	util::Movement _movement;
	scenegraph::KinematicBody _body;
	scenegraph::Physics _physics;
	double _deltaSeconds = 0.0;
	void moveCameraInEyeMode(video::Camera *camera, const scenegraph::SceneGraph &sceneGraph,
							 scenegraph::FrameIndex frameIdx);

public:
	void construct() override;
	bool init() override;
	void shutdown() override;
	/**
	 * @param[in] frameIdx If this is InvalidFrameIndex then no transform is applied
	 */
	void update(double nowSeconds, video::Camera *camera, const scenegraph::SceneGraph &sceneGraph,
				scenegraph::FrameIndex frameIdx);
	void zoom(video::Camera &camera, float level, double deltaSeconds);
	void pan(video::Camera &camera, int mouseDeltaX, int mouseDeltaY);
	/**
	 * @brief Updates the body position based on the camera's current position.
	 */
	void updateBodyPosition(const video::Camera &camera);
	const scenegraph::KinematicBody &body() const;
	scenegraph::KinematicBody &body();
};

inline const scenegraph::KinematicBody &CameraMovement::body() const {
	return _body;
}

inline scenegraph::KinematicBody &CameraMovement::body() {
	return _body;
}

} // namespace voxedit
