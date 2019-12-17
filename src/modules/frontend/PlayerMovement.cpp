/**
 * @file
 */

#include "PlayerMovement.h"
#include "core/command/Command.h"
#include "voxel/Constants.h"

namespace frontend {

void PlayerMovement::construct() {
	core::Command::registerActionButton("jump", _jump);
	core::Command::registerActionButton("move_forward", _moveForward);
	core::Command::registerActionButton("move_backward", _moveBackward);
	core::Command::registerActionButton("move_left", _moveLeft);
	core::Command::registerActionButton("move_right", _moveRight);
}

bool PlayerMovement::init() {
	return true;
}

void PlayerMovement::shutdown() {
	_deltaMillis = 0ul;
	core::Command::unregisterActionButton("jump");
	core::Command::unregisterActionButton("move_forward");
	core::Command::unregisterActionButton("move_backward");
	core::Command::unregisterActionButton("move_left");
	core::Command::unregisterActionButton("move_right");
	_jump.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveLeft.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveRight.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveForward.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveBackward.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
}

// TODO: get rid of this update method - merge with updatepos
void PlayerMovement::update(uint64_t deltaMillis) {
	_deltaMillis += deltaMillis;
}

void PlayerMovement::updatePos(float orientation, float deltaFrameSeconds, ClientEntityPtr& entity, std::function<int(const glm::vec3& pos)> heightResolver) {
	const attrib::ShadowAttributes& attribs = entity->attrib();
	const double speed = attribs.current(attrib::Type::SPEED);
	const glm::vec3& md = moveDelta(speed, orientation);
	const glm::vec3& currentPos = entity->position();

	glm::vec3 newPos = currentPos + md;
	_groundHeight = heightResolver(newPos);
	if (_groundHeight < voxel::MIN_HEIGHT) {
		_groundHeight = voxel::MIN_HEIGHT;
	}
	_delay -= deltaFrameSeconds;
	if (jump()) {
		if (_gliding) {
			if (_delay <= 0.0f) {
				_gliding = false;
				_jumping = true;
				_delay = 0.5f;
			}
		} else if (_jumping) {
			if (_delay <= 0.0f) {
				_jumping = false;
				_gliding = true;
				_delay = 0.5f;
			}
		} else {
			_velocityY = 10.0f;
			_jumping = true;
			_delay = 0.5f;
		}
	}
	const float gravity = _gliding ? 0.1f : 20.0f;
	_velocityY -= gravity * deltaFrameSeconds;
	newPos.y += _velocityY * deltaFrameSeconds;
	if (newPos.y <= _groundHeight) {
		newPos.y = _groundHeight;
		_velocityY = 0.0f;
		_jumping = false;
		_gliding = false;
		_delay = 0.0f;
	}
	if (_jumping) {
		entity->setAnimation(animation::Animation::Jump);
	} else if (_gliding) {
		entity->setAnimation(animation::Animation::Glide);
	} else if (moving()) {
		entity->setAnimation(animation::Animation::Run);
	} else {
		entity->setAnimation(animation::Animation::Idle);
	}
	entity->setPosition(newPos);
}

glm::vec3 PlayerMovement::moveDelta(float speed, float orientation) {
	if (_deltaMillis <= 0ul) {
		return glm::zero<glm::vec3>();
	}

	const glm::quat& rot = glm::angleAxis(orientation, glm::up);
	speed *= static_cast<float>(_deltaMillis) / 1000.0f;
	const glm::vec3& delta = calculateDelta(rot, speed);
	_deltaMillis = 0ul;
	return delta;
}

glm::vec3 PlayerMovement::calculateDelta(const glm::quat& rot, float speed) {
	if (_gliding || _jumping) {
		glm::vec3 delta(0.0f);
		if (forward()) {
			delta += rot * (glm::forward * speed);
		} else if (backward()) {
			// you can only reduce speed - but not walk backward
			delta += rot * (glm::forward * speed / 10.0f);
		}
		return delta;
	}
	glm::vec3 delta(0.0f);
	if (left()) {
		delta += rot * (glm::left * speed);
	} else if (right()) {
		delta += rot * (glm::right * speed);
	}
	if (forward()) {
		delta += rot * (glm::forward * speed);
	} else if (backward()) {
		delta += rot * (glm::backward * speed);
	}
	return delta;
}

}
