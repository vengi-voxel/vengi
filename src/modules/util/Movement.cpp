/**
 * @file
 */

#include "Movement.h"
#include "command/Command.h"
#include "math/Vec3f.h"
#include "math/Axes.h"

namespace util {

bool Movement::init() {
	return true;
}

void Movement::construct() {
	command::Command::registerActionButton("move_forward", _moveForward);
	command::Command::registerActionButton("move_backward", _moveBackward);
	command::Command::registerActionButton("move_left", _moveLeft);
	command::Command::registerActionButton("move_right", _moveRight);
	command::Command::registerActionButton("move_up", _moveUp);
	command::Command::registerActionButton("move_down", _moveDown);
	command::Command::registerActionButton("jump", _jump);
	command::Command::registerActionButton("sprint", _sprint);
}

void Movement::shutdown() {
	command::Command::unregisterActionButton("move_forward");
	command::Command::unregisterActionButton("move_backward");
	command::Command::unregisterActionButton("move_left");
	command::Command::unregisterActionButton("move_right");
	command::Command::unregisterActionButton("move_up");
	command::Command::unregisterActionButton("move_down");
	command::Command::unregisterActionButton("jump");
	command::Command::unregisterActionButton("sprint");
	_moveLeft.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveRight.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveForward.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveBackward.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveUp.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveDown.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_jump.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_sprint.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
}

void Movement::update(double nowSeconds) {
	updateDelta(nowSeconds);
}

math::Vec3f Movement::calculateDelta(double speed) const {
	math::Vec3f delta(0.0f);
	if (left()) {
		delta += (math::left() * (float)speed);
	} else if (right()) {
		delta += (math::right() * (float)speed);
	}
	if (forward()) {
		delta += (math::forward() * (float)speed);
	} else if (backward()) {
		delta += (math::backward() * (float)speed);
	}
	return delta;
}

math::Vec3f Movement::moveDelta(double speed) const {
	if (_deltaSeconds <= 0.0) {
		return math::Vec3f(0.0f);
	}
	return calculateDelta(speed * _deltaSeconds);
}

} // namespace util
