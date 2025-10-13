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

namespace voxedit {

class CameraMovement : public core::IComponent {
protected:
	core::VarPtr _movementSpeed;
	core::VarPtr _jumpVelocity;
	core::VarPtr _bodyHeight;
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
	void update(double nowSeconds, video::Camera *camera, const scenegraph::SceneGraph &sceneGraph,
				scenegraph::FrameIndex frameIdx);
	void zoom(video::Camera &camera, float level, double deltaSeconds);
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
