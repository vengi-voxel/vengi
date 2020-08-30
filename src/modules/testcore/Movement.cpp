/**
 * @file
 */

#include "Movement.h"
#include "command/Command.h"
#include "core/GLM.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/functions.hpp>

namespace testcore {

bool Movement::init() {
	return true;
}

void Movement::construct() {
	command::Command::registerActionButton("move_forward", _moveForward);
	command::Command::registerActionButton("move_backward", _moveBackward);
	command::Command::registerActionButton("move_left", _moveLeft);
	command::Command::registerActionButton("move_right", _moveRight);
}

void Movement::shutdown() {
	_deltaSeconds = 0.0;
	command::Command::unregisterActionButton("move_forward");
	command::Command::unregisterActionButton("move_backward");
	command::Command::unregisterActionButton("move_left");
	command::Command::unregisterActionButton("move_right");
	_moveLeft.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveRight.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveForward.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveBackward.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
}

void Movement::update(double deltaFrameSeconds) {
	_deltaSeconds += deltaFrameSeconds;
}

glm::vec3 Movement::calculateDelta(const glm::quat& rot, double speed) {
	glm::vec3 delta(0.0f);
	if (left()) {
		delta += rot * (glm::left * (float)speed);
	} else if (right()) {
		delta += rot * (glm::right * (float)speed);
	}
	if (forward()) {
		delta += rot * (glm::forward * (float)speed);
	} else if (backward()) {
		delta += rot * (glm::backward * (float)speed);
	}
	return delta;
}

glm::vec3 Movement::moveDelta(double speed, float orientation) {
	if (_deltaSeconds <= 0.0) {
		return glm::zero<glm::vec3>();
	}

	const glm::quat& rot = glm::angleAxis(orientation, glm::up);
	speed *= _deltaSeconds;
	const glm::vec3& delta = calculateDelta(rot, speed);
	_deltaSeconds = 0.0;
	return delta;
}

}
