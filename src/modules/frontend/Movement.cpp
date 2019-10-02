/**
 * @file
 */

#include "Movement.h"
#include "core/command/Command.h"
#include "core/GLM.h"

namespace frontend {

bool Movement::init() {
	return true;
}

void Movement::construct() {
	core::Command::registerActionButton("move_forward", _moveForward);
	core::Command::registerActionButton("move_backward", _moveBackward);
	core::Command::registerActionButton("move_left", _moveLeft);
	core::Command::registerActionButton("move_right", _moveRight);
}

void Movement::shutdown() {
	_deltaMillis = 0ul;
	core::Command::unregisterActionButton("move_forward");
	core::Command::unregisterActionButton("move_backward");
	core::Command::unregisterActionButton("move_left");
	core::Command::unregisterActionButton("move_right");
	_moveLeft.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveRight.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveForward.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveBackward.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
}

void Movement::update(uint64_t deltaMillis) {
	_deltaMillis += deltaMillis;
}

glm::vec3 Movement::calculateDelta(const glm::quat& rot, float speed) {
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

glm::vec3 Movement::moveDelta(float speed, float orientation) {
	if (_deltaMillis <= 0ul) {
		return glm::zero<glm::vec3>();
	}

	const glm::quat& rot = glm::angleAxis(orientation, glm::up);
	speed *= static_cast<float>(_deltaMillis) / 1000.0f;
	const glm::vec3& delta = calculateDelta(rot, speed);
	_deltaMillis = 0ul;
	return delta;
}

}
