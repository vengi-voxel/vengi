/**
 * @file
 */

#include "CameraMovement.h"
#include "app/I18N.h"
#include "core/Log.h"
#include "core/Var.h"
#include "scenegraph/Physics.h"

namespace voxelrender {

void CameraMovement::construct() {
	_movementSpeed = core::Var::get(cfg::GameModeMovementSpeed, "60.0", _("Movement speed in game mode"));
	_jumpVelocity = core::Var::get(cfg::GameModeJumpVelocity, "7.0", _("Jump velocity in game mode"));
	_bodyHeight = core::Var::get(cfg::GameModeBodyHeight, "2.0", _("Height of the body in game mode"));
	_clipping = core::Var::get(cfg::GameModeClipping, "false", core::CV_NOPERSIST, _("Enable camera clipping"),
							   core::Var::boolValidator);
	_applyGravity = core::Var::get(cfg::GameModeApplyGravity, "false", core::CV_NOPERSIST, _("Enable gravity"),
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
	const bool clipping = _clipping->boolVal();
	glm::vec3 camForward = camera->forward();
	glm::vec3 camRight = camera->right();
	if (clipping) {
		camForward.y = 0.0f;
		camRight.y = 0.0f;
	}
	_deltaSeconds += _movement.deltaSeconds();

	const bool isColliding = _body.isColliding();

	const float groundAcceleration = speed;
	const float airAcceleration = speed * 0.1f;
	const float acceleration = isColliding ? groundAcceleration : airAcceleration;
	const float maxSpeed = speed;

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

			// Apply acceleration in the desired direction
			const float accelAmount = acceleration * _deltaSeconds;
			glm::vec3 accelerationVec = direction * accelAmount;

			_body.velocity.x += accelerationVec.x;
			_body.velocity.z += accelerationVec.z;
			if (!clipping) {
				_body.velocity.y += accelerationVec.y;
			}

			// Cap the horizontal velocity to max allowed speed
			const float currentHorizontalSpeed = glm::length(glm::vec2(_body.velocity.x, _body.velocity.z));
			if (currentHorizontalSpeed > maxSpeed) {
				const float scale = maxSpeed / currentHorizontalSpeed;
				_body.velocity.x *= scale;
				_body.velocity.z *= scale;
				if (!clipping) {
					_body.velocity.y *= scale;
				}
			}
		}
	}

	// apply collision and gravity
	if (clipping) {
		const bool applyGravity = _applyGravity->boolVal();

		if (applyGravity && _movement.jump() && _body.isGrounded()) {
			_body.velocity.y = _jumpVelocity->floatVal();
			_body.collidedY = false;
		} else if (!applyGravity) {
			_body.velocity.y = 0.0f;
		}

		scenegraph::CollisionNodes nodes;
		sceneGraph.getCollisionNodes(nodes, frameIdx);

		constexpr double hz = 1.0 / 60.0;
		const float gravity = applyGravity ? 9.81f : 0.0f;
		while (_deltaSeconds > hz) {
			_physics.update(hz, nodes, _body, gravity);
			_deltaSeconds -= hz;
		}
		const float bodyHeight = _bodyHeight->floatVal();
		const float eyePos = bodyHeight * 0.9f;
		camera->setWorldPosition(_body.position + glm::vec3(0.0f, eyePos, 0.0f));
	} else {
		_body.position = camera->worldPosition();
		// no clipping - just move the camera directly
		_body.position += (_body.velocity * (float)_movement.deltaSeconds()) + camera->panOffset();
		camera->setWorldPosition(_body.position);
		_body.velocity = {0.0f, 0.0f, 0.0f};
	}
}

void CameraMovement::update(double nowSeconds, video::Camera *camera, const scenegraph::SceneGraph &sceneGraph,
							scenegraph::FrameIndex frameIdx) {
	_movement.update(nowSeconds);
	if (camera == nullptr) {
		return;
	}

	if (camera->rotationType() == video::CameraRotationType::Eye) {
		_body.extents.y = _bodyHeight->floatVal();
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
