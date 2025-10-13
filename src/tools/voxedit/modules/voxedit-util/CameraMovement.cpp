/**
 * @file
 */

#include "CameraMovement.h"
#include "app/I18N.h"
#include "core/Log.h"
#include "core/Var.h"
#include "voxedit-util/Config.h"

namespace voxedit {

// TODO: https://github.com/vengi-voxel/vengi/issues/634 add a player mode where you can
//       jump - specify the height of the player, the jump height, the velocity, etc.
//       it should be possible to walk a scene in a first person perspective
void CameraMovement::construct() {
	_movementSpeed = core::Var::get(cfg::VoxEditMovementSpeed, "180.0f");
	_clipping = core::Var::get(cfg::VoxEditClipping, "false", core::CV_NOPERSIST, _("Enable camera clipping"),
							   core::Var::boolValidator);
	_gravity =
		core::Var::get(cfg::VoxEditGravity, "false", core::CV_NOPERSIST, _("Enable gravity"), core::Var::boolValidator);
	_movement.construct();
}

bool CameraMovement::init() {
	if (!_movement.init()) {
		Log::error("Failed to initialize the movement controller");
		return false;
	}
	return true;
}

void CameraMovement::shutdown() {
	_movement.shutdown();
}

void CameraMovement::moveCameraInEyeMode(video::Camera *camera, const scenegraph::SceneGraph &sceneGraph,
										 scenegraph::FrameIndex frameIdx) {
	const float speed = _movementSpeed->floatVal();
	glm::vec3 moveDelta = _movement.moveDelta(speed);
	if (_clipping->boolVal()) {
		const glm::vec3 &camPos = camera->worldPosition();
		const glm::mat4 &orientation = camera->orientation();
		if (_gravity->boolVal()) {
			const float lowestY = sceneGraph.region().getLowerY() + _clipper.boxSize().y;
			const float currentY = camPos.y;
			moveDelta += _movement.gravityDelta(speed, orientation, currentY, lowestY);
		}
		voxelutil::RaycastResult result = _clipper.clipDelta(sceneGraph, frameIdx, camPos, moveDelta, orientation);
		moveDelta *= result.fract;
	}
	camera->move(moveDelta);
}

void CameraMovement::update(double nowSeconds, video::Camera *camera, const scenegraph::SceneGraph &sceneGraph,
							scenegraph::FrameIndex frameIdx) {
	_movement.update(nowSeconds);
	if (camera == nullptr) {
		return;
	}

	if (camera->rotationType() == video::CameraRotationType::Eye) {
		moveCameraInEyeMode(camera, sceneGraph, frameIdx);
	}
}

void CameraMovement::zoom(video::Camera &camera, float level, double deltaSeconds) {
	if (camera.rotationType() == video::CameraRotationType::Target) {
		camera.zoom(level);
	} else {
		float speed = level * _movementSpeed->floatVal();
		speed *= (float)deltaSeconds;
		camera.move(glm::vec3(0.0f, 0.0f, speed));
	}
}

} // namespace voxedit
