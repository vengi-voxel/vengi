/**
 * @file
 */

#include "Movement.h"
#include "command/Command.h"
#include "core/GLM.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtc/constants.hpp>
#include <glm/gtx/functions.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/mat4x4.hpp>

namespace util {

bool Movement::init() {
	return true;
}

void Movement::construct() {
	command::Command::registerActionButton("move_forward", _moveForward);
	command::Command::registerActionButton("move_backward", _moveBackward);
	command::Command::registerActionButton("move_left", _moveLeft);
	command::Command::registerActionButton("move_right", _moveRight);
	command::Command::registerActionButton("jump", _jump);
}

void Movement::shutdown() {
	command::Command::unregisterActionButton("move_forward");
	command::Command::unregisterActionButton("move_backward");
	command::Command::unregisterActionButton("move_left");
	command::Command::unregisterActionButton("move_right");
	command::Command::unregisterActionButton("jump");
	_moveLeft.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveRight.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveForward.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveBackward.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_jump.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
}

void Movement::update(double nowSeconds) {
	updateDelta(nowSeconds);
}

glm::vec3 Movement::calculateDelta(double speed) const {
	glm::vec3 delta(0.0f);
	if (left()) {
		delta += (glm::left() * (float)speed);
	} else if (right()) {
		delta += (glm::right() * (float)speed);
	}
	if (forward()) {
		delta += (glm::forward() * (float)speed);
	} else if (backward()) {
		delta += (glm::backward() * (float)speed);
	}
	return delta;
}

glm::vec3 Movement::gravityDelta(double speed, const glm::mat4 &orientation, float y, float lowestY) const {
	if (y <= lowestY) {
		return {0.0f, 0.0f, 0.0f};
	}
	glm::vec3 gravity{0.0f, -9.81 * _deltaSeconds, 0.0f};
	const glm::vec3 gravityDelta = orientation * glm::vec4(gravity, 0.0f);
	return gravityDelta;
}

glm::vec3 Movement::moveDelta(double speed) const {
	if (_deltaSeconds <= 0.0) {
		return glm::zero<glm::vec3>();
	}
	return calculateDelta(speed * _deltaSeconds);
}

} // namespace util
