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
	_gravity = core::Var::get(cfg::GameModeGravity, "9.81", _("Gravity in game mode"));
	_friction = core::Var::get(cfg::GameModeFriction, "0.01", _("Friction in game mode"));
	_bodySize = core::Var::get(cfg::GameModeBodySize, "0.2", _("Body size in game mode"));
	_clipping = core::Var::get(cfg::GameModeClipping, "false", core::CV_NOPERSIST, _("Enable camera clipping"),
							   core::Var::boolValidator);
	_applyGravity = core::Var::get(cfg::GameModeApplyGravity, "false", core::CV_NOPERSIST, _("Enable gravity"),
								   core::Var::boolValidator);
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
	_zoomSpeed = core::Var::getSafe(cfg::ClientCameraZoomSpeed);
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

void CameraMovement::update(double nowSeconds, video::Camera *camera, const scenegraph::SceneGraph &sceneGraph,
							scenegraph::FrameIndex frameIdx) {
	_movement.update(nowSeconds);
	if (camera == nullptr) {
		return;
	}

	_body.extents.y = _bodyHeight->floatVal();
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

			if (clipping) {
				const bool isColliding = _body.isColliding();
				const float groundAcceleration = speed;
				const float airAcceleration = speed * 0.1f;
				const float acceleration = isColliding ? groundAcceleration : airAcceleration;
				const float maxSpeed = speed;

				// Apply acceleration in the desired direction
				const float accelAmount = acceleration * _deltaSeconds;
				glm::vec3 accelerationVec = direction * accelAmount;

				_body.velocity.x += accelerationVec.x;
				_body.velocity.z += accelerationVec.z;

				// Cap the horizontal velocity to max allowed speed
				const float currentHorizontalSpeed = glm::length(glm::vec2(_body.velocity.x, _body.velocity.z));
				if (currentHorizontalSpeed > maxSpeed) {
					const float scale = maxSpeed / currentHorizontalSpeed;
					_body.velocity.x *= scale;
					_body.velocity.z *= scale;
				}
			} else {
				// In non-clipping mode, set velocity directly based on movement speed
				_body.velocity = direction * speed;
			}
		}
	}

	// apply collision and gravity
	if (clipping) {
		const bool applyGravity = _applyGravity->boolVal();
		_body.extents.x = _bodySize->floatVal();
		_body.extents.z = _body.extents.x;
		_body.frictionDecay = _friction->floatVal();

		if (applyGravity && _movement.jump() && _body.isGrounded()) {
			_body.velocity.y = _jumpVelocity->floatVal();
			_body.collidedY = false;
		} else if (!applyGravity) {
			_body.velocity.y = 0.0f;
		}

		scenegraph::CollisionNodes nodes;
		const float extentsFactor = 10.0f;
		const math::AABB<float> aabb(
			_body.position - _body.extents * extentsFactor,
			_body.position + _body.extents * extentsFactor
		);
		sceneGraph.getCollisionNodes(nodes, frameIdx, aabb);

		constexpr double hz = 1.0 / 60.0;
		const float gravity = applyGravity ? _gravity->floatVal() : 0.0f;
		while (_deltaSeconds > hz) {
			_physics.update(hz, nodes, _body, gravity);
			_deltaSeconds -= hz;
		}
		const float bodyHeight = _bodyHeight->floatVal();
		const float eyePos = bodyHeight * 0.9f;
		camera->setWorldPosition(_body.position + glm::vec3(0.0f, eyePos, 0.0f));
	} else {
		updateBodyPosition(*camera);
		// no clipping - just move the camera directly
		const glm::vec3 delta = (_body.velocity * (float)_deltaSeconds);
		_body.position += delta;
		camera->setTarget(camera->target() + delta);
		camera->setWorldPosition(camera->worldPosition() + delta);
		_body.velocity = {0.0f, 0.0f, 0.0f};
		_deltaSeconds = 0.0;
	}
}

void CameraMovement::pan(video::Camera &camera, int mouseDeltaX, int mouseDeltaY) {
	if (_clipping->boolVal()) {
		return;
	}

	camera.pan(mouseDeltaX, mouseDeltaY);
}

void CameraMovement::rotate(video::Camera &camera, float yaw, float pitch) {
	const float s = _rotationSpeed->floatVal();
	camera.turn(yaw * s);
	camera.setPitch(pitch * s);
}

void CameraMovement::zoom(video::Camera &camera, float level) {
	if (!_clipping->boolVal()) {
		if (camera.mode() == video::CameraMode::Orthogonal) {
			camera.zoom(level);
		} else {
			float speed = level * (1.0f + _zoomSpeed->floatVal());
			camera.move(glm::vec3(0.0f, 0.0f, speed));
		}
	}
}

} // namespace voxedit
