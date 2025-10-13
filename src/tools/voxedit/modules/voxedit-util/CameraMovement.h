/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/Var.h"
#include "scenegraph/Clipper.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/Movement.h"
#include "video/Camera.h"

namespace voxedit {

class CameraMovement : public core::IComponent {
protected:
	core::VarPtr _movementSpeed;
	core::VarPtr _gravity;
	core::VarPtr _clipping;
	scenegraph::Clipper _clipper;
	util::Movement _movement;
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

} // namespace voxedit
