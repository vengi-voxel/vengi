/**
 * @file
 */

#include "CameraMovement.h"
#include "app/I18N.h"
#include "core/Log.h"
#include "core/Var.h"
#include "scenegraph/Physics.h"
#include "voxedit-util/Config.h"

namespace voxedit {

void CameraMovement::construct() {
	_movementSpeed = core::Var::get(cfg::VoxEditMovementSpeed, "180.0");
	_jumpVelocity =
		core::Var::get(cfg::VoxEditJumpVelocity, "15.5", core::CV_NOPERSIST, _("Jump velocity in eye mode"));
	_bodyHeight =
		core::Var::get(cfg::VoxEditBodyHeight, "0.5", core::CV_NOPERSIST, _("Height of the body in eye mode"));
	_clipping = core::Var::get(cfg::VoxEditClipping, "false", core::CV_NOPERSIST, _("Enable camera clipping"),
							   core::Var::boolValidator);
	_applyGravity = core::Var::get(cfg::VoxEditApplyGravity, "false", core::CV_NOPERSIST, _("Enable gravity"),
								   core::Var::boolValidator);
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

void CameraMovement::updateBodyPosition(const video::Camera &camera) {
	_body.position = camera.worldPosition();
}

void CameraMovement::moveCameraInEyeMode(video::Camera *camera, const scenegraph::SceneGraph &sceneGraph,
										 scenegraph::FrameIndex frameIdx) {
	const float speed = _movementSpeed->floatVal();
	if (_clipping->isDirty()) {
		_clipping->markClean();
		updateBodyPosition(*camera);
	}
	// game mode - see Viewport::isGameMode()
	if (_clipping->boolVal()) {
		glm::vec3 camForward = camera->forward();
		glm::vec3 camRight = camera->right();
		camForward.y = 0.0f;
		camRight.y = 0.0f;
		_deltaSeconds += _movement.deltaSeconds();

		if (_movement.moving()) {
			glm::vec3 direction(0);
			if (_movement.forward()) {
				direction += camForward;
			}
			if (_movement.left()) {
				direction -= camRight;
			}
			if (_movement.backward()) {
				direction -= camForward;
			}
			if (_movement.right()) {
				direction += camRight;
			}
			if (glm::dot(direction, direction) > 0.0f) {
				direction = glm::normalize(direction);
				const float minmax = speed * _deltaSeconds;
				_body.velocity.x = glm::clamp(_body.velocity.x + direction.x, -minmax, minmax);
				_body.velocity.z = glm::clamp(_body.velocity.z + direction.z, -minmax, minmax);
			}
		}
		if (_applyGravity->boolVal() && _movement.jump() && _body.collidedY) {
			_body.velocity.y = _jumpVelocity->floatVal();
			_body.collidedY = false;
		} else if (!_applyGravity->boolVal()) {
			_body.velocity.y = 0.0f;
		}

		scenegraph::CollisionNodes nodes;
		sceneGraph.getCollisionNodes(nodes, frameIdx);

		constexpr double hz = 1.0 / 60.0;
		const float gravity = _applyGravity->boolVal() ? 9.81f : 0.0f;
		while (_deltaSeconds > hz) {
			_physics.update(hz, nodes, _body, gravity);
			_deltaSeconds -= hz;
		}
		const float bodyHeight = _bodyHeight->floatVal();
		camera->setWorldPosition(_body.position + glm::vec3(0.0f, bodyHeight, 0.0f));
	} else {
		glm::vec3 moveDelta = _movement.moveDelta(speed);
		camera->move(moveDelta);
	}
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
	} else if (!_clipping->boolVal()) {
		float speed = level * _movementSpeed->floatVal();
		speed *= (float)deltaSeconds;
		camera.move(glm::vec3(0.0f, 0.0f, speed));
		camera.update(deltaSeconds);
	}
}

} // namespace voxedit
